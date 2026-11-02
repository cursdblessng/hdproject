// core data types shared by every file, plus building the world
#pragma once

#include "splashkit.h"
#include <cstdint>

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

/**
 * all block types, air is for empty array slot
 * new world is all air
 */
enum BlockType:uint8_t
{
    AIR,
    GRASS,
    DIRT,
    STONE,
    WOOD,
    LEAVES,
    WATER,
    SAND,
};

struct Camera
{
    Vector3 position;
    double yaw;   // rotation for left right
    double pitch; // rotation for up down
    double fov;
};

const int CHUNK_SIZE = 16; // chunk is 16 x 16 blocks
const int WORLD_HEIGHT = 128; // nothing can be placed after 127
const int WORLD_CHUNKS = 32; // world is 32 x 32 chunks, 1024
const int WORLD_SIZE = CHUNK_SIZE * WORLD_CHUNKS; // 512 blocks x 512 blocks

/**
 * 1 chunk is 16 x 128 x 16 blocks
 * blocks[x][y][z]
 */
struct Chunk
{
    BlockType blocks[CHUNK_SIZE][WORLD_HEIGHT][CHUNK_SIZE];
};

/**
 * 32 x 32 chunks
 */
struct World
{
    Chunk chunks[WORLD_CHUNKS][WORLD_CHUNKS];
};

// the grid is at y = 0, treat it as solid ground: you stand on it and
// can place blocks on it. used by collision (input.cpp) and the raycast (world.cpp)
const double WORLD_FLOOR_Y = 0.0;

// window size when not fullscreen. used when the window first opens and
// when coming back out of fullscreen, so the two always match
const int WINDOWED_WIDTH = 800;
const int WINDOWED_HEIGHT = 600;

// frame rate cap passed to refresh_screen. 60 for normal play,
// 0 = no cap, for measuring how fast the renderer really is.
// note: movement is per frame, so with no cap the game runs too fast
const unsigned int TARGET_FPS = 60;

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
    bool hit_block; // true if block is hit
    int block_x, block_y, block_z; // coords of hit block
    bool can_place; // true if ray hit a block within reach
    int place_x, place_y, place_z; // empty coords the placed block will go to
};

// picks display color for blocks
color block_colour(BlockType type);

// checks if given coordinates are in any world arrays
bool in_world(int x, int y, int z);

// returns block type at given coordinates
BlockType get_block(const World &world, int x, int y, int z);

// sets a block type at given coordinates
void set_block(World &world, int x, int y, int z, BlockType type);

// sets all blocks in the world to air
void clear_world(World &world);

// can this block be stood on, collided with and hit by the crosshair (air and water can't)
bool is_solid(BlockType type);

// works out the 8 corner positions of a block
CubeCorners get_block_corners(int x, int y, int z);

// fills the world with flat square. size x size, 5 blocks down
// 1 grass, 2 dirt, 2 stone
void build_superflat(World &world, int size);

// fills the whole world with perlin noise hills from a seed:
// stone, then dirt, then grass on top. same seed = same world
void build_terrain(World &world, unsigned int seed);

// camera position for the player to start at: on dry land, as close to the middle as possible
Vector3 find_spawn(const World &world);

// do two boxes overlap
bool boxes_overlap(const Box &a, const Box &b);

// the space a block fills, from its min corner position to min corner + 1
Box block_box(int x, int y, int z);

// does this block overlap any active block? used for player collision
bool box_hits_world(const Box &box, const World &world);

// what the crosshair is pointing at: which block, and where a new block would go
RayHit raycast_block(const Camera &cam, const World &world);