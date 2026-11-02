// core types shared by every file, the block grid, terrain generation, collision and the raycast
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
 * every block type, air is an empty cell.
 * uint8_t so each block is 1 byte, which keeps the world at 32 mb.
 * saves store these numbers, so new types go on the end, otherwise blocks in old saves would load as a different type
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
    Vector3 position; // the eye, for the player this is 1.62 above their feet
    double yaw;       // turning left and right, in radians
    double pitch;     // looking up and down in radians, positive = looking down
    double fov;       // how wide the view is left to right, in radians
};

const int CHUNK_SIZE = 16;                        // a chunk is 16 x 16 blocks across
const int WORLD_HEIGHT = 128;                     // y goes from 0 to 127
const int WORLD_CHUNKS = 32;                      // the world is 32 x 32 chunks, 1024 in total
const int WORLD_SIZE = CHUNK_SIZE * WORLD_CHUNKS; // 512 x 512 blocks across

/**
 * 16 x 128 x 16 blocks, the full height of the world
 * blocks[x][y][z], with x and z from 0 to 15 inside the chunk
 */
struct Chunk
{
    BlockType blocks[CHUNK_SIZE][WORLD_HEIGHT][CHUNK_SIZE];
};

/**
 * 32 x 32 chunks, 512 x 128 x 512 blocks.
 * 32 mb, much bigger than the 1 mb stack, so main makes it static
 */
struct World
{
    Chunk chunks[WORLD_CHUNKS][WORLD_CHUNKS];
};

// what a world is built from, same settings build same world
struct WorldSettings
{
    string name;
    unsigned int seed; // not used by superflat
    bool is_superflat;
};

// how many worlds can be kept at once
const int NUM_SAVE_SLOTS = 3;

// one save slot on the world select screen
struct SaveSlot
{
    bool is_used;           // false = empty slot, settings are left over and mean nothing
    WorldSettings settings;
};

// every save slot wrapped in a struct so it can be passed by ref
struct SaveSlots
{
    SaveSlot slots[NUM_SAVE_SLOTS];
};

// the bottom of the world counts as solid ground: you can't fall through it
// and blocks can be placed on it. used by collision (input.cpp) and the raycast (world.cpp)
const double WORLD_FLOOR_Y = 0.0;

// window size when not fullscreen. used when the window first opens and
// when coming back out of fullscreen, so the two always match
const int WINDOWED_WIDTH = 800;
const int WINDOWED_HEIGHT = 600;

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
 * all the space between a min corner and a max corner.
 * its edges always run along x, y and z (never rotated), so checking if two overlap is cheap.
 * used for collision: a block is 1 x 1 x 1, the player is 0.6 x 1.8 x 0.6
 */
struct Box
{
    Vector3 min; // smallest x, y and z corner
    Vector3 max; // largest x, y and z corner
};

// what the crosshair ray found
struct RayHit
{
    bool hit_block;                // a solid block was hit within reach
    int block_x, block_y, block_z; // the block that was hit
    bool can_place;                // a block or the floor was hit within reach, so there's somewhere to place
    int place_x, place_y, place_z; // the empty cell just in front of what was hit, where a new block goes
};

// the colour a block type is drawn in
color block_colour(BlockType type);

// is this cell inside the world
bool in_world(int x, int y, int z);

// the block in a cell, anything outside the world is air
BlockType get_block(const World &world, int x, int y, int z);

// changes the block in a cell, cells outside the world are ignored
void set_block(World &world, int x, int y, int z, BlockType type);

// sets all blocks in the world to air
void clear_world(World &world);

// can this block be stood on, collided with and hit by the crosshair (air and water can't)
bool is_solid(BlockType type);

// works out the 8 corner positions of a block
CubeCorners get_block_corners(int x, int y, int z);

// a flat size x size square in the middle of the world, 5 blocks deep:
// 2 stone, 2 dirt, 1 grass
void build_superflat(World &world, int size);

// the whole world from perlin noise: plains, mountains, oceans, rivers, beaches and trees.
// same seed = same world
void build_terrain(World &world, unsigned int seed);

// empties the world and builds it from its settings
void build_world(World &world, const WorldSettings &settings);

// camera position for the player to start at: on dry land, as close to the middle as possible
Vector3 find_spawn(const World &world);

// do two boxes overlap
bool boxes_overlap(const Box &a, const Box &b);

// the space a block fills, from its min corner position to min corner + 1
Box block_box(int x, int y, int z);

// does this box overlap any solid block? used for player collision
bool box_hits_world(const Box &box, const World &world);

// what the crosshair is pointing at: which block, and where a new block would go
RayHit raycast_block(const Camera &cam, const World &world);
