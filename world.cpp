#include "world.h"
#include <cmath>

// how far away you can reach to break a block
const double REACH = 5.0;

// how far the ray moves each check. smaller means more accurate but slower
const double RAY_STEP = 0.05;

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
 * what is the crosshair pointing at?
 * walks from the camera along the look direction in small steps
 * stepping outwards means the first hit is always the nearest block
 * so you cant hit a block hidden behind a closer one
 * also remembers the last point that was in empty space:
 * when something is hit, the point is right in front of the face
 * you're looking at, so its cell is where a new block should be placed.
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

    // count steps with an int rather than adding RAY_STEP to a double over and over
    // so tiny rounding errors dont build up along the way
    int num_steps = (int)(REACH / RAY_STEP);

    // coords of the last cell that wasnt solid
    // starts at the eye cell which cant be solid cuz collision
    int prev_x = (int)floor(cam.position.x);
    int prev_y = (int)floor(cam.position.y);
    int prev_z = (int)floor(cam.position.z);

    for (int s = 0; s <= num_steps; s++)
    {
        // dir has length 1, so this point is exactly 'distance' blocks away
        double distance = s * RAY_STEP;
        Vector3 point = {cam.position.x + dir.x * distance,
                         cam.position.y + dir.y * distance,
                         cam.position.z + dir.z * distance};

        // went through the floor: nothing to break, but a block can go on it
        if (point.y < WORLD_FLOOR_Y)
        {
            result.can_place = true;
            result.place_x = prev_x;
            result.place_y = prev_y;
            result.place_z = prev_z;
            return result;
        }

        // which cell the point is in
        int cell_x = (int)floor(point.x);
        int cell_y = (int)floor(point.y);
        int cell_z = (int)floor(point.z);

        // one lookup instead of looping over every block
        if (get_block(world, cell_x, cell_y, cell_z) != AIR)
        {
            result.hit_block = true;
            result.block_x = cell_x;
            result.block_y = cell_y;
            result.block_z = cell_z;

            result.can_place = true;
            result.place_x = prev_x;
            result.place_y = prev_y;
            result.place_z = prev_z;
            return result;
        }

        // this cell was empty, so it becomes the new "last empty cell"
        prev_x = cell_x;
        prev_y = cell_y;
        prev_z = cell_z;
    }

    return result; // reached REACH without hitting anything, still "nothing found"
}