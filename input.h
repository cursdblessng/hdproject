// keyboard and mouse controls for the camera
#pragma once

#include "world.h"

/**
 * movement state for the player that isnt stored in the camera
 * (the camera's position IS the players eyes, so position lives there)
 * kept between frames because gravity builds up speed over time
 */
struct Player
{
    double velocity_y; // up/down speed in blocks per frame, negative = falling
    bool on_ground;    // standing on something, so allowed to jump
    bool is_flying;    // true = no gravity, space/shift move up and down
};

// wasd movement relative to where camera is facing, space/shift for up/down
void handle_movement(Camera &cam, Player &player, const BlockArray &world);

// mouse look, only called when the cursor is locked
void handle_mouse_look(Camera &cam, double mouse_sensitivity);

// escape key toggles whether or not cursor is locked
void handle_cursor_toggle(bool &is_cursor_locked);

// f toggles flying on/off
void handle_fly_toggle(Player &player);

// left click breaks the block the crosshair is on
void handle_block_break(const Camera &cam, BlockArray &world);

// numbers 1-6 pick which block is placed
void handle_block_select(BlockType &selected_block);

// right click places the block against whatever crosshair is on
void handle_block_place(const Camera &cam, BlockArray &world, BlockType selected_block);