// core data types shared by every file, plus building the world
#pragma once

#include "splashkit.h" // for the color type

struct Vector3
{
    double x;
    double y;
    double z;
};

struct Vector2
{
    double x;
    double y;
};

enum BlockType
{
    GRASS,
    DIRT,
    STONE,
    WOOD,
    LEAVES,
    WATER,
};

struct Block
{
    Vector3 position;
    BlockType type;
    bool is_active;
};

struct Camera
{
    Vector3 position;
    double yaw;   // rotation for left right
    double pitch; // rotation for up down
    double fov;
};

const int MAX_BLOCKS = 10000;

struct BlockArray
{
    Block data[MAX_BLOCKS];
    int size;
};

/**
 * the corners of a cube, in a struct so it can be
 * returned from functions without using pointers
 *
 * index layout:
 * bottom face (y + 0):  0 = (0,0,0)  1 = (1,0,0)  2 = (1,0,1)  3 = (0,0,1)
 * top face    (y + 1):  4 = (0,1,0)  5 = (1,1,0)  6 = (1,1,1)  7 = (0,1,1)
 *
 * corner i is directly below i + 4
 */
struct CubeCorners
{
    Vector3 points[8];
};

/**
 * an axis-aligned box: all the space between a min corner and a max corner
 * "axis-aligned" means its edges always run along x, y and z (never rotated),
 * which makes checking whether two boxes overlap very cheap
 * used for collision: a block is a 1x1x1 box, the player is 0.6 x 2 x 0.6
 */
struct Box
{
    Vector3 min; // smallest x, y and z corner
    Vector3 max; // largest x, y and z corner
};

// empties the block array so blocks can be added from index 0
void init_block_array(BlockArray &arr);

// adds a block to the end of the array, if there's room
void add_block(BlockArray &arr, Block b);

// picks display color for blocks
color block_colour(BlockType type);

// works out the 8 corner positions of a block
CubeCorners get_block_corners(const Block &b);

// fills the world with the hand placed test blocks
void build_test_world(BlockArray &world);

// does this block overlap any active block? used for player collision
bool box_hits_world(const Box &box, const BlockArray &world);

// index of the block the crosshair is pointing at, or -1 if none within reach
int raycast_block(const Camera &cam, const BlockArray &world);