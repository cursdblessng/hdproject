#include "world.h"
#include <cmath>

// how far away you can reach to break a block
const double REACH = 5.0;

// how far the ray moves each check. smaller means more accurate but slower
const double RAY_STEP = 0.05;

void init_block_array(BlockArray &arr)
{
    arr.size = 0;
}

void add_block(BlockArray &arr, Block b)
{
    if (arr.size < MAX_BLOCKS)
    {
        arr.data[arr.size] = b;
        arr.size++;
    }
    else
    {
        write_line("Maximum block count reached.");
    }
}

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
    }
    return COLOR_WHITE; // fallback (should never happen)
}

// works out the 8 corner positions of a block
CubeCorners get_block_corners(const Block &b)
{
    CubeCorners c;

    double x = b.position.x;
    double y = b.position.y;
    double z = b.position.z;

    // clang-format off
    // bottom face of cube
    c.points[0] = {x,       y, z};
    c.points[1] = {x + 1.0, y, z};
    c.points[2] = {x + 1.0, y, z + 1.0};
    c.points[3] = {x,       y, z + 1.0};

    // top face, y values +1 because its above
    c.points[4] = {x,       y + 1.0, z};
    c.points[5] = {x + 1.0, y + 1.0, z};
    c.points[6] = {x + 1.0, y + 1.0, z + 1.0};
    c.points[7] = {x,       y + 1.0, z + 1.0};
    // clang-format on

    return c;
}

void build_test_world(BlockArray &world)
{
    // 5 x 5 patch of grass
    for (int x = -2; x <= 2; x++)
    {
        for (int z = -2; z <= 2; z++)
        {
            add_block(world, {{(double)x, 0.0, (double)z}, GRASS, true});
        }
    }

    // 3 high wood block with 1 leaf block on top
    add_block(world, {{4.0, 0.0, 4.0}, WOOD, true});
    add_block(world, {{4.0, 1.0, 4.0}, WOOD, true});
    add_block(world, {{4.0, 2.0, 4.0}, WOOD, true});
    add_block(world, {{4.0, 3.0, 4.0}, LEAVES, true});

    // stone block floating on the sise
    add_block(world, {{-4.0, 5.0, 3.0}, STONE, true});
}

/**
 * fills world with flat square. size x size, 5 blocks down
 * y = 0 to 1: stone. 2 to 3: dirt. 4: grass. surface at y = 5
 * centered around the origin so spawn is 0,0
 */
void build_superflat(BlockArray &world, int size)
{
    // start at -size/2 so the square is centered on 0,
    // so something like size 20 goes from -10 to 9
    int start = -size / 2;

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

                add_block(world, {{(double)x, (double)y, (double)z}, type, true});
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
Box block_box(const Block &b)
{
    Box box;
    box.min = b.position;
    box.max = {b.position.x + 1.0, b.position.y + 1.0, b.position.z + 1.0};

    return box;
}

/**
 * does this box overlap any active block in the world?
 * checks every block, stops as soon as one hits
 */
bool box_hits_world(const Box &box, const BlockArray &world)
{
    for (int i = 0; i < world.size; i++)
    {
        if (world.data[i].is_active && boxes_overlap(box, block_box(world.data[i])))
        {
            return true;
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
 * is this point inside a block?
 * the block fills position to position + 1 on every axis.
 * >= at the start and < at the end, so that
 * a point exactly between two blocks counts as inside one only, not both
 */
bool point_in_block(const Vector3 &p, const Block &b)
{
    return p.x >= b.position.x && p.x < b.position.x + 1.0 &&
           p.y >= b.position.y && p.y < b.position.y + 1.0 &&
           p.z >= b.position.z && p.z < b.position.z + 1.0;
}

/**
 * the min corner of the 1x1x1 grid cell this point is in
 * floor (not a plain int cast) because (int)-0.5 is 0.0
 * which is the wrong cell for negative numbers. floor(-0.5) is -1
 */
Vector3 cell_of(const Vector3 &p)
{
    return {floor(p.x), floor(p.y), floor(p.z)};
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
RayHit raycast_block(const Camera &cam, const BlockArray &world)
{
    // start with 'nothing found', only changed if something is hit
    RayHit result;
    result.block_index = -1;
    result.can_place = false;
    result.place_position = {0.0, 0.0, 0.0};

    Vector3 dir = look_direction(cam);

    // count steps with an int rather than adding RAY_STEP to a double over and over
    // so tiny rounding errors dont build up along the way
    int num_steps = (int)(REACH / RAY_STEP);

    // the last point that wasnt inside anything.
    // starts at the eye, which can never be inside a block bc collision
    Vector3 previous = cam.position;

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
            result.place_position = cell_of(previous);
            return result;
        }

        for (int i = 0; i < world.size; i++)
        {
            if (world.data[i].is_active && point_in_block(point, world.data[i]))
            {
                result.block_index = i;
                result.can_place = true;
                result.place_position = cell_of(previous);
                return result;
            }
        }

        // this point was empty, so it becomes the new "last empty point"
        previous = point;
    }

    return result; // reached REACH without hitting anything, still "nothing found"
}