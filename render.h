// drawing the world: projection, clipping, wireframe and solid blocks
#pragma once

#include "world.h"
#include "timing.h"

// grey ground grid on y = 0, grid_radius lines out from the origin
void draw_grid(const Camera &cam, int grid_radius);

// every active block as shaded solid faces (painter's algorithm)
void draw_world_solid(const Camera &view_cam, const Camera &cull_cam, const World &world, bool see_through, double time_of_day, FrameTimer &timer);

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