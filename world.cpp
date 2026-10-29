#include "world.h"

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