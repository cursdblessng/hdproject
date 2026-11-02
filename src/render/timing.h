// measures how long each part of a frame takes, to find out what makes it slow
#pragma once

// the parts of a frame that get timed
enum TimedStage
{
    STAGE_COLLECT, // collect_visible_faces: scanning cells and culling
    STAGE_SORT,    // sort_faces_far_to_close
    STAGE_PROJECT, // draw_face maths: projecting and clipping the corners
    STAGE_FILL,    // splashkit fill_triangle calls
    STAGE_OUTLINE, // the 4 draw_3d_line calls around each face
    STAGE_PRESENT, // refresh_screen: putting the finished frame on screen
    NUM_STAGES     // not a stage, enums count up from 0 so this is how many there are
};

struct FrameTimer
{
    double stage_ms[NUM_STAGES]; // time spent in each stage since the last report
    double frame_start_ms;       // when the current frame started
    double frame_total_ms;       // whole frames added up, so the untimed parts can be worked out
    int frames;                  // frames since the last report
    long faces;                  // faces drawn since the last report
    double last_report_ms;       // when the last report was printed
};

// milliseconds since some fixed point, with fractions of a millisecond
double time_now_ms();

// a timer with everything at 0
FrameTimer make_frame_timer();

// adds the time from start_ms until now to a stage
void add_stage_time(FrameTimer &timer, TimedStage stage, double start_ms);

// call at the very start of each frame
void start_timed_frame(FrameTimer &timer);

// call at the very end of each frame, prints the averages about once a second
void end_timed_frame(FrameTimer &timer);
