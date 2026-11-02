// day and night: the time of day, the sky colour, how bright blocks are, and the sun and moon
#pragma once

#include "world.h"

// moves the time of day (0 to 1, a whole day) on by dt seconds, back to 0 after a full day
void advance_time(double &time_of_day, double day_length_seconds, double dt);

// how bright block faces are at this time of day, dim at midnight up to 1 at noon
double block_light(double time_of_day);

// fills the screen with the sky colour for this time of day, then draws the sun and moon
void draw_sky(const Camera &cam, double time_of_day);
