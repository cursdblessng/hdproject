// keyboard and mouse controls for the camera
#pragma once

#include "world.h"

// wasd movement relative to where camera is facing, space/shift for up/down
void handle_movement(Camera &cam, const BlockArray &world);

// mouse look, only called when the cursor is locked
void handle_mouse_look(Camera &cam, double mouse_sensitivity);

// escape key toggles whether or not cursor is locked
void handle_cursor_toggle(bool &is_cursor_locked);
