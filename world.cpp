#include "world.h"
#include <cmath>

// stb_perlin is a one-file library: normally the header only declares its functions.
// defining this first makes it include the actual code as well.
// must be in exactly ONE .cpp file, otherwise the functions exist twice and linking fails
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

// how far away you can reach to break a block
const double REACH = 5.0;

/**
 * stands in for "never" on an axis the ray doesn't move along
 * (e.g. dir.x = 0 when looking straight down). much bigger than REACH,
 * so that axis is never picked as the next edge to cross.
 * used instead of dividing by 0
 */
const double RAY_NEVER = 1e30;

// terrain shape
const int TERRAIN_BASE_HEIGHT = 40;    // ground level of the flat areas
const double PLAINS_HEIGHT = 12.0;     // gentle hills everywhere go this far up/down
const double PLAINS_SCALE = 96.0;      // roughly how wide (in blocks) those hills are
const double MOUNTAIN_HEIGHT = 60.0;   // the highest a mountain can rise above the plains
const double MOUNTAIN_SCALE = 64.0;    // how far apart the mountain ridges are
const double MOUNTAIN_SHARPNESS = 3.0; // power on the ridges: higher = narrower, steeper peaks
const double REGION_SCALE = 256.0;     // size of the areas that are flat vs mountainous
const int TERRAIN_OCTAVES = 4;         // layers of detail, see fractal_noise
const int DIRT_DEPTH = 3;              // dirt blocks under the grass, stone below that
const double CLIFF_SLOPE = 2.5;        // slopes this steep or more are bare stone, lower = more stone

// noise is higher for more extreme biomes
const int NOISE_LAYER_PLAINS = 0;
const int NOISE_LAYER_REGIONS = 1;
const int NOISE_LAYER_MOUNTAINS = 2;

// picks display color for blocks
color block_colour(BlockType type)
{
    switch (type)
    {
    case GRASS:
        return rgb_color(95, 159, 53);
    case DIRT:
        return rgb_color(134, 96, 67);
    case STONE:
        return rgb_color(125, 125, 125);
    case WOOD:
        return rgb_color(102, 81, 51);
    case LEAVES:
        return rgb_color(40, 110, 30);
    case WATER:
        return rgb_color(50, 90, 200);
    case AIR:
        break; // air not drawn so use fallback
    }
    return COLOR_WHITE; // fallback
}

/**
 * returns bool
 * is block cell inside the worlds arrays
 * x & z: 0-511
 * y: 0-127
 */
bool in_world(int x, int y, int z)
{
    return x >= 0 && x < WORLD_SIZE &&
           y >= 0 && y < WORLD_HEIGHT &&
           z >= 0 && z < WORLD_SIZE;
}

/**
 * checks what block is at given coordinates
 * returns air if theres no block in any world arrays
 * otherwise returns block type
 */
BlockType get_block(const World &world, int x, int y, int z)
{
    if (!in_world(x, y, z))
    {
        return AIR;
    }

    return world.chunks[x / CHUNK_SIZE][z / CHUNK_SIZE].blocks[x % CHUNK_SIZE][y][z % CHUNK_SIZE];
}

/**
 * puts a block type in the given coordinates
 * breaking block sets said coordinates as air type
 */
void set_block(World &world, int x, int y, int z, BlockType type)
{
    if (!in_world(x, y, z))
    {
        return;
    }

    world.chunks[x / CHUNK_SIZE][z / CHUNK_SIZE].blocks[x % CHUNK_SIZE][y][z % CHUNK_SIZE] = type;
}

// works out the 8 corner positions of a block
CubeCorners get_block_corners(int x, int y, int z)
{
    CubeCorners c;

    double fx = x;
    double fy = y;
    double fz = z;

    // clang-format off
    // bottom face of cube
    c.points[0] = {fx,       fy, fz};
    c.points[1] = {fx + 1.0, fy, fz};
    c.points[2] = {fx + 1.0, fy, fz + 1.0};
    c.points[3] = {fx,       fy, fz + 1.0};

    // top face, y values +1 because its above
    c.points[4] = {fx,       fy + 1.0, fz};
    c.points[5] = {fx + 1.0, fy + 1.0, fz};
    c.points[6] = {fx + 1.0, fy + 1.0, fz + 1.0};
    c.points[7] = {fx,       fy + 1.0, fz + 1.0};
    // clang-format on

    return c;
}

/**
 * fills world with flat square. size x size, 5 blocks down
 * y = 0 to 1: stone. 2 to 3: dirt. 4: grass. surface at y = 5
 * centered around the origin so spawn is 0,0
 */
void build_superflat(World &world, int size)
{
    // start at -size/2 so the square is centered on 0,
    // so something like size 20 goes from 246 to 265
    int start = WORLD_SIZE / 2 - size / 2;

    for (int x = start; x < start + size; x++)
    {
        for (int z = start; z < start + size; z++)
        {
            for (int y = 0; y < 5; y++)
            {
                // bottom two layers stone, then two dirt, then grass
                BlockType type;
                if (y <= 1)
                {
                    type = STONE;
                }
                else if (y <= 3)
                {
                    type = DIRT;
                }
                else
                {
                    type = GRASS;
                }

                set_block(world, x, y, z, type);
            }
        }
    }
}

/**
 * one layer of 2d noise around -1 to 1
 */
float noise_2d(double x, double z, double frequency, unsigned int seed, int layer)
{
    int slice = ((int)((seed >> 8) & 255) + layer * 85) & 255;

    return stb_perlin_noise3_seed((float)(x * frequency), (float)slice + 0.5f, (float)(z * frequency),
                                  0, 0, 0, (int)(seed & 255));
}

/**
 * noise added up in octaves
 * each octave has more detail than the last
 * but amplitude is half
 * scale is how wide the shapes are
 */
double fractal_noise(int x, int z, double scale, unsigned int seed, int layer)
{
    double total = 0.0;
    double max_total = 0.0; // biggest total could be
    double frequency = 1.0 / scale;
    double amplitude = 1.0;

    for (int i = 0; i < TERRAIN_OCTAVES; i++)
    {
        total += noise_2d(x, z, frequency, seed, layer) * amplitude;
        max_total += amplitude;

        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return total / max_total;
}

// keeps a number between 0 and 1
double clamp_01(double value)
{
    if (value < 0.0)
    {
        return 0.0;
    }
    if (value > 1.0)
    {
        return 1.0;
    }
    return value;
}

/**
 * how high the ground is at a column x,z before rounding to a whole block
 * plains everywhere, with mountains added on top in mountainous regions
 * exact heights change smoothly so the slope check uses these
 */
double terrain_height_exact(int x, int z, unsigned int seed)
{
    double plains = fractal_noise(x, z, PLAINS_SCALE, seed, NOISE_LAYER_PLAINS);

    /**
     * region noise is mostly between -0.5 and 0.5
     * so after *2, lots of areas reach 0 or 1 (flat or mountains) after clamping
     * +0.3 makes mountains more common
     * then smoothstep eases in and out so terrain transitions smoothly
     */
    double region = noise_2d(x, z, 1.0 / REGION_SCALE, seed, NOISE_LAYER_REGIONS);
    double mountain_amount = clamp_01(region * 2.0 + 0.3);
    mountain_amount = mountain_amount * mountain_amount * (3.0 - 2.0 * mountain_amount);

    /**
     * ridges: noise crosses 0 along wiggly lines. |noise| is 0 there with a sharp V shape,
     * so 1 - |noise| is 1 (a peak) along those lines. the power keeps peaks at 1 but
     * shrinks everything else (0.5^3 = 0.125), giving narrow ridges with steep sides
     */
    double ridge = 1.0 - fabs(fractal_noise(x, z, MOUNTAIN_SCALE, seed, NOISE_LAYER_MOUNTAINS));
    ridge = pow(ridge, MOUNTAIN_SHARPNESS);

    return TERRAIN_BASE_HEIGHT + plains * PLAINS_HEIGHT + mountain_amount * ridge * MOUNTAIN_HEIGHT;
}

/**
 * the y of the top block in column x,z
 * exact height rounded down to a whole block
 * floor so it always rounds down, int alone would round towards 0
 */
int terrain_height(int x, int z, unsigned int seed)
{
    return (int)floor(terrain_height_exact(x, z, seed));
}

/**
 * how steep the ground is at column x,z, in blocks of height per block across
 * uses exact heights so columns on the same slope get almost the same value,
 * rounded heights step 2, 3, 2, 3 down a slope and would only turn every other column to stone
 * compares the neighbours on both sides of each axis, which are 2 blocks apart so halved
 * pythagoras combines the x and z slopes into the slope in the steepest direction
 * uses terrain height not world because neighbour columns may not be built yet
 */
double terrain_slope(int x, int z, unsigned int seed)
{
    double slope_x = (terrain_height_exact(x + 1, z, seed) - terrain_height_exact(x - 1, z, seed)) / 2.0;
    double slope_z = (terrain_height_exact(x, z + 1, seed) - terrain_height_exact(x, z - 1, seed)) / 2.0;

    return sqrt(slope_x * slope_x + slope_z * slope_z);
}

/**
 * fills the whole world with the noised terrain
 * each column from the bottom is stone, then dirt depth, then 1 grass
 * world starts as all air so only the solid parts need setting
 * same seed always gives same world
 */
void build_terrain(World &world, unsigned int seed)
{
    for (int x = 0; x < WORLD_SIZE; x++)
    {
        for (int z = 0; z < WORLD_SIZE; z++)
        {
            // one noise lookup per column, not per block
            int surface = terrain_height(x, z, seed);

            // worked out once per column, not per block
            // steep enough that grass and dirt couldnt stay on it
            bool is_cliff = terrain_slope(x, z, seed) >= CLIFF_SLOPE;

            for (int y = 0; y <= surface; y++)
            {
                BlockType type;
                if (is_cliff)
                {
                    // whole column, so the cliff face shows no dirt layers either
                    type = STONE;
                }
                else if (y == surface)
                {
                    type = GRASS;
                }
                else if (y >= surface - DIRT_DEPTH)
                {
                    type = DIRT;
                }
                else
                {
                    type = STONE;
                }

                set_block(world, x, y, z, type);
            }
        }
    }
}

int surface_y(const World &world, int x, int z)
{
    for (int y = WORLD_HEIGHT - 1; y >= 0; y--)
    {
        if (get_block(world, x, y, z) != AIR)
        {
            return y + 1;
        }
    }

    return 0;
}

/**
 * do two boxes overlap?
 * on one axis, two ranges overlap if each one starts before the other ends.
 * boxes only overlap if that's true on x, y AND z at the same time.
 * < and > (not <= >=) so boxes that are just touching don't count,
 * otherwise standing exactly on a block would count as being inside it
 */
bool boxes_overlap(const Box &a, const Box &b)
{
    return a.min.x < b.max.x && a.max.x > b.min.x &&
           a.min.y < b.max.y && a.max.y > b.min.y &&
           a.min.z < b.max.z && a.max.z > b.min.z;
}

// the space that a block fills, from its position (min corner) to position + 1
Box block_box(int x, int y, int z)
{
    Box box;
    box.min = {(double)x, (double)y, (double)z};
    box.max = {x + 1.0, y + 1.0, z + 1.0};

    return box;
}

/**
 * does this box overlap any active block in the world?
 * checks every block, stops as soon as one hits
 */
bool box_hits_world(const Box &box, const World &world)
{
    int min_x = (int)floor(box.min.x);
    int min_y = (int)floor(box.min.y);
    int min_z = (int)floor(box.min.z);

    int max_x = (int)ceil(box.max.x) - 1;
    int max_y = (int)ceil(box.max.y) - 1;
    int max_z = (int)ceil(box.max.z) - 1;

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            for (int z = min_z; z <= max_z; z++)
            {
                // if its not air then its solid
                if (get_block(world, x, y, z) != AIR)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * the direction the camera is looking, as a 3D direction of length 1.
 * starts from the flat forward direction (same as handle_movement),
 * then tilts it by pitch:
 * - looking down (positive pitch) makes y negative, by sin(pitch)
 * - the more you tilt, the less of the ray points forward, so x and z
 *   shrink by cos(pitch) (straight down = no forward part at all)
 */
Vector3 look_direction(const Camera &cam)
{
    Vector3 dir;
    dir.x = -sin(cam.yaw) * cos(cam.pitch);
    dir.y = -sin(cam.pitch);
    dir.z = cos(cam.yaw) * cos(cam.pitch);
    return dir;
}

/**
 * what the DDA raycast tracks on ONE axis (x, y or z).
 * the ray uses three of these, and each moves on its own
 */
struct RayAxis
{
    int cell;       // which cell the ray is in on this axis
    int step;       // +1 or -1: which way the ray moves on this axis (0 = doesn't move)
    double t_delta; // distance along the ray to cross one whole cell on this axis
    double t_max;   // distance along the ray to reach the NEXT cell edge on this axis
};

/**
 * sets up one axis from the camera position and look direction on that axis
 * t_max is the distance to the next edge in this axis, * t_delta to turn it into distance along the ray
 */
RayAxis make_ray_axis(double pos, double dir)
{
    RayAxis axis;
    axis.cell = (int)floor(pos);

    if (dir > 0)
    {
        // moving up the axis: next edge is the far side of this cell so cell + 1
        axis.step = 1;
        axis.t_delta = 1.0 / dir;
        axis.t_max = (axis.cell + 1 - pos) * axis.t_delta;
    }
    else if (dir < 0)
    {
        // moving down the axis: next edge is the min side
        axis.step = -1;
        axis.t_delta = -1.0 / dir;
        axis.t_max = (pos - axis.cell) * axis.t_delta;
    }

    return axis;
}

/**
 * crosses the next edge on this axis: moves one cell along it and works out
 * where the edge after that is (one t_delta further).
 * returns how far along the ray that edge was, so the raycast can check it against REACH
 */
double step_axis(RayAxis &axis)
{
    double distance = axis.t_max;
    axis.cell += axis.step;
    axis.t_max += axis.t_delta;
    return distance;
}

/**
 * what is the crosshair pointing at?
 * 3D DDA: instead of checking points every small step, the ray jumps straight
 * to the next cell edge it crosses. each axis counts down to its next edge
 * (t_max), and whichever edge is closest gets crossed, moving one cell
 * on that axis only. this means:
 * - every cell the ray touches is checked, so corners can't be skipped
 * - each cell shares a FACE with the one before it, so the previous (empty)
 *   cell is always a valid spot to place a block, never diagonal
 * cells are visited nearest first, so the first solid one is the block
 * you're looking at, not one hidden behind it
 */
RayHit raycast_block(const Camera &cam, const World &world)
{
    // start with 'nothing found' so only changed if something is hit
    RayHit result;
    result.hit_block = false;
    result.block_x = 0;
    result.block_y = 0;
    result.block_z = 0;
    result.can_place = false;
    result.place_x = 0;
    result.place_y = 0;
    result.place_z = 0;

    Vector3 dir = look_direction(cam);

    // one countdown per axis, all starting from the eye's cell
    RayAxis ax = make_ray_axis(cam.position.x, dir.x);
    RayAxis ay = make_ray_axis(cam.position.y, dir.y);
    RayAxis az = make_ray_axis(cam.position.z, dir.z);

    /**
     * this always ends: every t_delta is at least 1 (no part of dir is bigger
     * than 1), so each crossing pushes one countdown at least 1 further.
     * after a few crossings even the smallest is past REACH, and it returns
     */
    while (true)
    {
        // the cell the ray is about to leave. it was checked last loop and was
        // empty (or it's the eye cell, which collision keeps empty), and it
        // shares a face with the next cell, so it's where a block would go
        int prev_x = ax.cell;
        int prev_y = ay.cell;
        int prev_z = az.cell;

        // cross whichever edge comes first along the ray.
        // <= so a tie (ray exactly through an edge or corner) still picks one,
        // either is fine since both cells touch the ray there
        double distance;
        if (ax.t_max <= ay.t_max && ax.t_max <= az.t_max)
        {
            distance = step_axis(ax);
        }
        else if (ay.t_max <= az.t_max)
        {
            distance = step_axis(ay);
        }
        else
        {
            distance = step_axis(az);
        }

        // the edge we just crossed is out of reach, so nothing was found
        if (distance > REACH)
        {
            return result;
        }

        // went below the grid: nothing to break, but a block can go on it
        if (ay.cell < WORLD_FLOOR_Y)
        {
            result.can_place = true;
            result.place_x = prev_x;
            result.place_y = prev_y;
            result.place_z = prev_z;
            return result;
        }

        // one lookup for the cell the ray just entered
        if (get_block(world, ax.cell, ay.cell, az.cell) != AIR)
        {
            result.hit_block = true;
            result.block_x = ax.cell;
            result.block_y = ay.cell;
            result.block_z = az.cell;

            result.can_place = true;
            result.place_x = prev_x;
            result.place_y = prev_y;
            result.place_z = prev_z;
            return result;
        }
    }

    return result; // reached REACH without hitting anything so result is in nothing found state
}