// the game as a whole: everything it remembers between frames, and what each screen does in one frame
#pragma once

#include "../world/world.h"
#include "input.h"            // Player
#include "settings.h"
#include "../ui/menu.h"       // CreateWorldForm
#include "../ui/hud.h"        // FpsCounter
#include "../render/timing.h" // FrameTimer

// which screen is showing, decides what each frame does
enum GameScreen
{
    SCREEN_MAIN_MENU,
    SCREEN_WORLD_SELECT,
    SCREEN_CREATE_WORLD,
    SCREEN_DELETE_WORLD,
    SCREEN_SETTINGS,
    SCREEN_PLAYING
};

/**
 * everything the game remembers from one frame to the next, apart from the world's blocks.
 * in one struct so main can hand all of it to a screen with one reference.
 * the world is 32 mb, too big for the stack, so main keeps it static and passes it separately
 */
struct Game
{
    // which screen is showing
    GameScreen screen;
    GameScreen settings_return_screen; // where done on the settings screen goes back to
    bool should_quit;                  // exit was clicked on the main menu

    // the player's settings, read from settings.txt
    Settings settings;
    int settings_page; // which page of settings is showing, stays on the last one used

    // the save slots and the screens that change them
    SaveSlots save_slots;
    int current_slot;            // which slot is being played, so quitting knows where to save
    int delete_slot;             // which slot the delete world screen is asking about
    CreateWorldForm create_form; // what's been typed and picked on the create world screen

    // the world being played
    Camera cam;               // the player's eyes
    Player player;
    BlockType selected_block; // which block right click places, changed with keys 1-7
    double time_of_day;       // 0 to 1: 0 = sunrise, 0.25 = noon, 0.5 = sunset, 0.75 = midnight
    bool is_cursor_locked;    // locked = playing, unlocked = paused or on a menu

    // debug views
    bool see_through;        // half transparent faces, toggled with x
    bool is_camera_detached; // c detaches the view to fly around the world, the player stays frozen
    Camera debug_cam;        // the camera used while detached

    // timing
    double last_frame_ms;   // when the last frame started, for working out dt
    FpsCounter fps_counter; // the fps shown in the top left
    FrameTimer frame_timer; // times each part of a frame, printed in the terminal every second
};

// the game as it first opens: on the main menu, with the settings and save slots read in
Game make_game();

// timing, events and f11, at the start of every frame on every screen.
// returns dt, the seconds since the last frame
double start_frame(Game &game);

// one frame of each screen: handles its clicks and keys, then draws it.
// changing game.screen moves to another screen from the next frame
void update_main_menu(Game &game);
void update_world_select(Game &game, World &world);
void update_create_world(Game &game, World &world);
void update_delete_world(Game &game);
void update_settings(Game &game);
void update_playing(Game &game, World &world, double dt);

// writes the world being played to its slot
void save_current_world(const Game &game, const World &world);
