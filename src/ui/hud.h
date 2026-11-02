// the hud drawn over the world: crosshair, selected block, fps counter, debug info and the debug camera label
#pragma once

#include "../world/world.h"

// frame rate measurement, kept in main because it has to remember
// things between frames
struct FpsCounter
{
    unsigned int last_update_ticks; // current_ticks() when fps was last worked out
    int frames_since_update;        // frames drawn since then
    double fps;                     // the number shown on screen
};

// a counter starting from now, with nothing counted yet
FpsCounter make_fps_counter();

// call once per frame, counts the frame, and every half second works out the fps
void update_fps_counter(FpsCounter &counter);

// draws the fps in the top left corner
void draw_fps_counter(const FpsCounter &counter);

// faces drawn, which block the camera is in and the time of day, under the fps counter
void draw_debug_info(const Camera &cam, int faces_drawn, double time_of_day);

// red dot and "DEBUG CAMERA" in the bottom right corner, drawn while the camera is detached
void draw_debug_cam_label();

// small white dot in the middle of the screen
void draw_crosshair();

// the selected block's colour in the bottom left, so you can tell what you're holding
void draw_selected_block(BlockType selected_block);
