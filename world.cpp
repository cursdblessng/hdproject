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
