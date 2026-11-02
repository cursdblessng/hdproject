// drawing the blocks: frustum and face culling, then sorting the faces far to near and filling them in
#pragma once

#include "world.h"
#include "timing.h"
#include "settings.h"

// the most faces that can be drawn in one frame. normal play is around 3000.
// the list is on the stack (10000 * 24 bytes = 240 kb), much bigger would overflow the 1 mb stack
const int MAX_VISIBLE_FACES = 10000;

// every block as shaded solid faces (painter's algorithm), returns how many faces were collected.
// the culling and outline settings decide what gets drawn
int draw_world_solid(const Camera &view_cam, const Camera &cull_cam, const World &world, bool see_through, double time_of_day, const Settings &settings, FrameTimer &timer);

// the edges of the player's view, seen from the debug camera
void draw_player_frustum(const Camera &view_cam, const Camera &player_cam);

// blue tint over the whole screen while the camera is underwater
void draw_underwater_tint(const Camera &cam, const World &world);
