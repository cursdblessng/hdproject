#include "sky.h"
#include "projection.h"
#include "splashkit.h"
#include <cmath> // sin, cos, fmod

const double PI = 3.14159265358979323846;

// sky colour at noon and at midnight, everything in between is a mix of the two
const color DAY_SKY_COLOUR = rgb_color(135, 206, 235);
const color NIGHT_SKY_COLOUR = rgb_color(10, 12, 30);

// how bright blocks are at midnight compared to noon.
// not 0, otherwise the world would be pitch black and you couldn't see anything
const double NIGHT_LIGHT = 0.25;

// how far the sun's height (-1 to 1) goes either side of the horizon while it fades between day and night.
// 0.25 is about 15 degrees, so with a 60 second day the fade takes about 5 seconds
const double TWILIGHT = 0.25;

// how far away the sun and moon are drawn, and how big they are at that distance.
// 8 at 100 away is about 9 degrees across for the sun
const double SKY_DISTANCE = 100.0;
const double SUN_RADIUS = 8.0;
const double MOON_RADIUS = 6.0;
const color SUN_COLOUR = rgb_color(255, 230, 120);
const color MOON_COLOUR = rgb_color(220, 220, 235);

/**
 * moves the time of day on by dt seconds.
 * dt / day_length is the fraction of a whole day that passed this frame,
 * e.g. 1/60 of a second in a 60 second day moves it on 1/3600
 */
void advance_time(double &time_of_day, double day_length_seconds, double dt)
{
    time_of_day += dt / day_length_seconds;

    // fmod keeps only the part after the decimal point, so 1.02 becomes 0.02, early the next day
    time_of_day = fmod(time_of_day, 1.0);
}

/**
 * which way the sun is, from anywhere in the world. length 1.
 * goes round in a circle up through east (+x), overhead, then down through west (-x):
 * 0 = sunrise, 0.25 = noon, 0.5 = sunset, 0.75 = midnight.
 * cos and sin of the same angle always make a length 1 direction
 */
Vector3 sun_direction(double time_of_day)
{
    double angle = time_of_day * 2.0 * PI;
    return {cos(angle), sin(angle), 0.0};
}

/**
 * 0 at night, 1 in the day, fading in between while the sun is near the horizon.
 * the sun's y is its height: 1 at noon, 0 on the horizon, -1 at midnight
 */
double daylight_amount(double time_of_day)
{
    double sun_height = sun_direction(time_of_day).y;

    // -TWILIGHT -> 0, 0 -> 0.5, +TWILIGHT -> 1
    double amount = (sun_height + TWILIGHT) / (2.0 * TWILIGHT);

    // outside the fade it would go below 0 or above 1, so keep it in range
    if (amount < 0.0)
    {
        amount = 0.0;
    }
    if (amount > 1.0)
    {
        amount = 1.0;
    }
    return amount;
}

// how bright block faces are right now, NIGHT_LIGHT at night up to 1 in the day
double block_light(double time_of_day)
{
    return NIGHT_LIGHT + (1.0 - NIGHT_LIGHT) * daylight_amount(time_of_day);
}

// mixes two colours, amount 0 = all of a, 1 = all of b, 0.5 = halfway between
color mix_colours(color a, color b, double amount)
{
    return rgb_color((int)(red_of(a) + (red_of(b) - red_of(a)) * amount),
                     (int)(green_of(a) + (green_of(b) - green_of(a)) * amount),
                     (int)(blue_of(a) + (blue_of(b) - blue_of(a)) * amount));
}

/**
 * a circle in the sky in that direction from the camera, for the sun and the moon.
 * placed SKY_DISTANCE from the camera wherever it goes, so you can never walk
 * any closer to it, just like something really far away.
 * drawn before the world, so blocks in front cover it
 */
void draw_sky_disc(const Camera &cam, const Vector3 &direction, color colour, double radius)
{
    // below the horizon: the ground should be hiding it, but nothing is drawn
    // past the edge of the render distance, so it would show through
    if (direction.y < -0.1)
    {
        return;
    }

    Vector3 sky_point = {cam.position.x + direction.x * SKY_DISTANCE,
                         cam.position.y + direction.y * SKY_DISTANCE,
                         cam.position.z + direction.z * SKY_DISTANCE};

    Vector3 c = world_to_camera(cam, sky_point);

    // behind the camera
    if (c.z < NEAR_PLANE)
    {
        return;
    }

    Vector2 centre = camera_to_screen(cam, c);

    // a point radius above the centre, the gap between the two on screen is how big it looks.
    // the edge is higher up the screen, so its y is smaller
    Vector2 edge = camera_to_screen(cam, {c.x, c.y + radius, c.z});
    double screen_radius = centre.y - edge.y;

    // completely off the window. near the side of the view the pixel numbers get huge,
    // which would overflow splashkit's drawing the same way the faces did
    if (centre.x < -screen_radius || centre.x > screen_width() + screen_radius ||
        centre.y < -screen_radius || centre.y > screen_height() + screen_radius)
    {
        return;
    }

    fill_circle(colour, centre.x, centre.y, screen_radius);
}

void draw_sky(const Camera &cam, double time_of_day)
{
    clear_screen(mix_colours(NIGHT_SKY_COLOUR, DAY_SKY_COLOUR, daylight_amount(time_of_day)));

    Vector3 sun = sun_direction(time_of_day);

    // the moon is always on the opposite side of the sky, so it rises as the sun sets
    Vector3 moon = {-sun.x, -sun.y, -sun.z};

    draw_sky_disc(cam, sun, SUN_COLOUR, SUN_RADIUS);
    draw_sky_disc(cam, moon, MOON_COLOUR, MOON_RADIUS);
}
