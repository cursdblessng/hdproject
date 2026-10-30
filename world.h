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

// the grid is at y = 0, treat it as solid ground: you stand on it and
// can place blocks on it. used by collision (input.cpp) and the raycast (world.cpp)
const double WORLD_FLOOR_Y = 0.0;

// window size when not fullscreen. used when the window first opens and
// when coming back out of fullscreen, so the two always match
const int WINDOWED_WIDTH = 800;
const int WINDOWED_HEIGHT = 600;

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

// what the crosshair ray found
struct RayHit
{
    int block_index;        // block that was hit, -1 if none (or only the floor was hit)
    bool can_place;         // true if the ray hit a block or the floor within reach
    Vector3 place_position; // min corner of the empty cell a new block would go in
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

// do two boxes overlap
bool boxes_overlap(const Box &a, const Box &b);

// the space a block fills, from its min corner position to min corner + 1
Box block_box(const Block &b);

// does this block overlap any active block? used for player collision
bool box_hits_world(const Box &box, const BlockArray &world);

// what the crosshair is pointing at: which block, and where a new block would go
RayHit raycast_block(const Camera &cam, const BlockArray &world);