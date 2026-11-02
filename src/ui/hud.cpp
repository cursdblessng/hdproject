#include "hud.h"
#include "../render/render.h" // MAX_VISIBLE_FACES
#include "menu.h"             // MENU_FONT and LABEL_SIZE, for the debug camera label
#include "splashkit.h"
#include <cmath> // floor

// FPS counter was AI Assisted
// debug camera hud was AI assisted

// how often the fps number is recalculated.
// averaging over half a second gives a steady number instead of one that flickers every frame
const unsigned int FPS_UPDATE_MS = 500;

FpsCounter make_fps_counter()
{
    FpsCounter counter;
    counter.last_update_ticks = current_ticks();
    counter.frames_since_update = 0;
    counter.fps = 0.0;
    return counter;
}

/**
 * counts this frame, then once FPS_UPDATE_MS has passed works out
 * fps = frames / seconds, which is frames * 1000 / milliseconds.
 * 1000.0 (not 1000) so the division is done with decimals,
 * otherwise int division would chop off the fraction
 * // AI assisted
 */
void update_fps_counter(FpsCounter &counter)
{
    counter.frames_since_update++;

    unsigned int now = current_ticks();
    unsigned int elapsed = now - counter.last_update_ticks;

    if (elapsed >= FPS_UPDATE_MS)
    {
        counter.fps = counter.frames_since_update * 1000.0 / elapsed;

        // start counting the next half second
        counter.frames_since_update = 0;
        counter.last_update_ticks = now;
    }
}

// draws the fps in the top left corner, rounded down to a whole number
void draw_fps_counter(const FpsCounter &counter)
{
    draw_text("FPS: " + std::to_string((int)counter.fps), COLOR_WHITE, 10, 10);
}
// a number as at least 2 digits, 7 -> "07", for the clock
string two_digits(int number)
{
    if (number < 10)
    {
        return "0" + std::to_string(number);
    }
    return std::to_string(number);
}

/**
 * small text under the fps counter, in the same built in font.
 * faces shows how close the list is to full, so turning culling off shows why it's needed.
 * time is shown as a clock: time_of_day 0 is sunrise, so 6:00 is added on
 */
void draw_debug_info(const Camera &cam, int faces_drawn, double time_of_day)
{
    string faces = "Faces: " + std::to_string(faces_drawn) + " / " + std::to_string(MAX_VISIBLE_FACES);

    // the list filled up, so some faces that should be drawn are missing
    if (faces_drawn >= MAX_VISIBLE_FACES)
    {
        faces += " FULL";
    }

    string block = "Block: " + std::to_string((int)floor(cam.position.x)) + " " +
                   std::to_string((int)floor(cam.position.y)) + " " +
                   std::to_string((int)floor(cam.position.z));

    // minutes since midnight, 1440 in a day. % 1440 wraps it past midnight
    int minutes = ((int)(time_of_day * 1440.0) + 6 * 60) % 1440;
    string clock = "Time: " + two_digits(minutes / 60) + ":" + two_digits(minutes % 60);

    draw_text(faces, COLOR_WHITE, 10, 25);
    draw_text(block, COLOR_WHITE, 10, 40);
    draw_text(clock, COLOR_WHITE, 10, 55);
}

// how far the debug camera label sits in from the window's edges
const double HUD_MARGIN = 10.0;
const double DEBUG_DOT_RADIUS = 6.0;

// space inside the label's box, around the dot and text and between them
const double DEBUG_LABEL_PADDING = 8.0;

/**
 * dark see-through box in the bottom right corner with a red dot and red text,
 * the box keeps the red readable over bright sky and blocks.
 * the box is sized from the text, then everything is placed from its top left corner
 */
void draw_debug_cam_label()
{
    string text = "DEBUG CAMERA";
    double width = text_width(text, MENU_FONT, LABEL_SIZE);
    double height = text_height(text, MENU_FONT, LABEL_SIZE);

    // padding, dot, padding, text, padding
    double dot_space = DEBUG_DOT_RADIUS * 2.0 + DEBUG_LABEL_PADDING;
    double box_width = DEBUG_LABEL_PADDING + dot_space + width + DEBUG_LABEL_PADDING;
    double box_height = height + DEBUG_LABEL_PADDING * 2.0;

    // measured back from the bottom right corner
    double box_x = screen_width() - HUD_MARGIN - box_width;
    double box_y = screen_height() - HUD_MARGIN - box_height;

    fill_rectangle(rgba_color(0, 0, 0, 150), box_x, box_y, box_width, box_height);

    // the dot and the text both line up on the box's middle height
    double centre_y = box_y + box_height / 2.0;
    fill_circle(COLOR_RED, box_x + DEBUG_LABEL_PADDING + DEBUG_DOT_RADIUS, centre_y, DEBUG_DOT_RADIUS);
    draw_text(text, COLOR_RED, MENU_FONT, LABEL_SIZE, box_x + DEBUG_LABEL_PADDING + dot_space, centre_y - height / 2.0);
}

// what the crosshair ray aims at
void draw_crosshair()
{
    fill_circle(COLOR_WHITE, screen_width() / 2.0, screen_height() / 2.0, 2);
}

/**
 * a black square 2 pixels bigger on every side goes underneath first,
 * so the edge left showing is a border that stands out on any background
 */
void draw_selected_block(BlockType selected_block)
{
    fill_rectangle(COLOR_BLACK, 8, screen_height() - 42, 34, 34);
    fill_rectangle(block_colour(selected_block), 10, screen_height() - 40, 30, 30);
}
