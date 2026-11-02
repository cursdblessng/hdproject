#include "timing.h"
#include <chrono> // steady_clock
#include <cstdio> // printf, fflush

// debugging was AI assisted

// how often the averages are printed
const double REPORT_INTERVAL_MS = 1000.0;

/**
 * steady_clock only ever goes forward, unlike the computer's clock which can jump
 * when it syncs, so differences between two readings are always real time.
 * duration<double, milli> turns it into milliseconds as a double, keeping the fractions
 */
double time_now_ms()
{
    std::chrono::duration<double, std::milli> since_start = std::chrono::steady_clock::now().time_since_epoch();
    return since_start.count();
}

// a timer with everything at 0, in a function because the array needs a loop
FrameTimer make_frame_timer()
{
    FrameTimer timer;

    for (int i = 0; i < NUM_STAGES; i++)
    {
        timer.stage_ms[i] = 0.0;
    }

    timer.frame_start_ms = time_now_ms();
    timer.frame_total_ms = 0.0;
    timer.frames = 0;
    timer.faces = 0;
    timer.last_report_ms = time_now_ms();

    return timer;
}

/**
 * adds the time since start_ms to a stage.
 * the enum value is used as the array index, so each stage has its own slot
 */
void add_stage_time(FrameTimer &timer, TimedStage stage, double start_ms)
{
    timer.stage_ms[stage] += time_now_ms() - start_ms;
}

void start_timed_frame(FrameTimer &timer)
{
    timer.frame_start_ms = time_now_ms();
}

/**
 * counts the frame, then about once a second prints each stage's average per frame.
 * averaged over a second because single frames jump around too much to read.
 * other = whole frame minus every timed stage: input, the sky, the hud.
 * fflush because the msys2 terminal holds printf output back until there's a lot of it,
 * so without it nothing shows up while the game is running
 */
void end_timed_frame(FrameTimer &timer)
{
    double now = time_now_ms();
    timer.frame_total_ms += now - timer.frame_start_ms;
    timer.frames++;

    if (now - timer.last_report_ms < REPORT_INTERVAL_MS)
    {
        return;
    }

    double frames = timer.frames;

    double timed_ms = 0.0;
    for (int i = 0; i < NUM_STAGES; i++)
    {
        timed_ms += timer.stage_ms[i];
    }
    double other_ms = timer.frame_total_ms - timed_ms;

    printf("collect %5.1f | sort %4.1f | project %5.1f | fill %5.1f | outline %5.1f | present %5.1f | other %4.1f | frame %5.1f ms | faces %ld\n",
           timer.stage_ms[STAGE_COLLECT] / frames,
           timer.stage_ms[STAGE_SORT] / frames,
           timer.stage_ms[STAGE_PROJECT] / frames,
           timer.stage_ms[STAGE_FILL] / frames,
           timer.stage_ms[STAGE_OUTLINE] / frames,
           timer.stage_ms[STAGE_PRESENT] / frames,
           other_ms / frames,
           timer.frame_total_ms / frames,
           timer.faces / timer.frames);
    fflush(stdout);

    // start the next second's totals from 0
    for (int i = 0; i < NUM_STAGES; i++)
    {
        timer.stage_ms[i] = 0.0;
    }
    timer.frame_total_ms = 0.0;
    timer.frames = 0;
    timer.faces = 0;
    timer.last_report_ms = now;
}
