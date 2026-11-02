// drawing the world: projection, clipping, solid blocks, the sky and the hud
#pragma once

#include "world.h"
#include "timing.h"
#include "settings.h"

// grey ground grid on y = 0, grid_radius lines out from the origin
void draw_grid(const Camera &cam, int grid_radius);

// every block as shaded solid faces (painter's algorithm), returns how many faces were collected.
// the culling and outline settings decide what gets drawn
int draw_world_solid(const Camera &view_cam, const Camera &cull_cam, const World &world, bool see_through, double time_of_day, const Settings &settings, FrameTimer &timer);

// the edges of the player's view, seen from the debug camera
void draw_player_frustum(const Camera &view_cam, const Camera &player_cam);

// fills the screen with the sky colour for this time of day, then draws the sun and moon
void draw_sky(const Camera &cam, double time_of_day);

// blue tint over the whole screen while the camera is underwater
void draw_underwater_tint(const Camera &cam, const World &world);

// frame rate measurement, kept in main because it has to remember
// things between frames
struct FpsCounter
{
    unsigned int last_update_ticks; // current_ticks() when fps was last worked out
    int frames_since_update;        // frames drawn since then
    double fps;                     // the number shown on screen
};

// call once per frame, counts the frame, and every half second works out the fps
void update_fps_counter(FpsCounter &counter);

// draws the fps in the top left corner
void draw_fps_counter(const FpsCounter &counter);

// faces drawn, which block the camera is in and the time of day, under the fps counter
void draw_debug_info(const Camera &cam, int faces_drawn, double time_of_day);
