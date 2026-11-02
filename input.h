// keyboard and mouse controls: moving the player, looking, toggles, and breaking and placing blocks
#pragma once

#include "world.h"

/**
 * movement state for the player that isnt stored in the camera
 * (the camera's position is the player's eyes, so position lives there)
 * kept between frames because gravity builds up speed over time
 */
struct Player
{
    double velocity_y; // up/down speed in blocks per second, negative = falling
    bool on_ground;    // standing on something, so allowed to jump
    bool is_flying;    // true = no gravity, space/shift move up and down
};

// wasd movement relative to where the camera is facing, with gravity and collision.
// space jumps, or flies up with shift for down. dt = seconds since last frame
void handle_movement(Camera &cam, Player &player, const World &world, double dt);

// mouse look, only called when the cursor is locked
void handle_mouse_look(Camera &cam, double mouse_sensitivity);

// hides the mouse and locks it to the middle, for playing
void lock_cursor(bool &is_cursor_locked);

// escape locks and unlocks the cursor, unlocked = paused
void handle_cursor_toggle(bool &is_cursor_locked);

// f toggles flying on/off
void handle_fly_toggle(Player &player);

// left click breaks the block the crosshair is on
void handle_block_break(const Camera &cam, World &world);

// numbers 1-7 pick which block is placed
void handle_block_select(BlockType &selected_block);

// right click places the block against whatever the crosshair is on
void handle_block_place(const Camera &cam, World &world, BlockType selected_block);

// f11 toggles between fullscreen and windowed
void handle_fullscreen_toggle(bool is_cursor_locked);

// x toggles see-through faces (debug view)
void handle_see_through_toggle(bool &see_through);

// c detaches the view into a free flying debug camera (starts at the player's view)
void handle_camera_detach_toggle(bool &is_detached, Camera &debug_cam, const Camera &player_cam);

// flies a camera around with no gravity or collision (debug camera), dt = seconds since last frame
void handle_free_fly(Camera &cam, double dt);