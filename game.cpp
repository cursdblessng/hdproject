#include "game.h"
#include "render.h"
#include "sky.h"
#include "save.h"
#include "splashkit.h"

// a frame longer than this (dragging the window, a big hitch) is treated as this long,
// so the player doesn't suddenly jump a long way after the game stalls
const double MAX_FRAME_SECONDS = 0.25;

// ============================================================
// SETTING UP AND EVERY FRAME
// ============================================================

Game make_game()
{
    Game game;

    game.screen = SCREEN_MAIN_MENU;
    game.settings_return_screen = SCREEN_MAIN_MENU;
    game.should_quit = false;

    // the defaults if there's no settings.txt yet
    game.settings = load_settings();
    game.settings_page = 0;

    game.save_slots = load_save_slots();
    game.current_slot = 0;
    game.delete_slot = 0;
    game.create_form = make_create_world_form(0);

    // moved to the spawn, or wherever it was saved, when a world is started
    game.cam.position = {0.0, 0.0, 0.0};
    game.cam.yaw = 0.0;
    game.cam.pitch = 0.0;
    game.cam.fov = chosen_fov(game.settings);

    // starts on foot, press f to fly
    game.player.velocity_y = 0.0;
    game.player.on_ground = false;
    game.player.is_flying = false;

    game.selected_block = GRASS;

    // starts in the morning so the first world opens in daylight
    game.time_of_day = 0.1;

    // unlocked so the mouse can click the menu
    game.is_cursor_locked = false;

    game.see_through = false;
    game.is_camera_detached = false;
    game.debug_cam = game.cam; // overwritten on detach, set here so it's never garbage

    game.last_frame_ms = time_now_ms();
    game.fps_counter = make_fps_counter();
    game.frame_timer = make_frame_timer();

    return game;
}

double start_frame(Game &game)
{
    start_timed_frame(game.frame_timer);

    // seconds since the last frame, used to make movement per second instead of per frame
    double now_ms = time_now_ms();
    double dt = (now_ms - game.last_frame_ms) / 1000.0;
    game.last_frame_ms = now_ms;

    if (dt > MAX_FRAME_SECONDS)
    {
        dt = MAX_FRAME_SECONDS;
    }

    process_events();

    // before the screens so f11 works on every one
    handle_fullscreen_toggle(game.is_cursor_locked);

    return dt;
}

void save_current_world(const Game &game, const World &world)
{
    save_world(game.current_slot, game.save_slots.slots[game.current_slot].settings, world, game.cam, game.player);
}

/**
 * opens the slot's save if it has one. if not, builds a new world from the slot's settings
 * and saves it straight away, so the slot has files from the start.
 * a damaged save is rebuilt from its settings too, so the world still opens.
 * everything else is reset so nothing carries over from the last world played
 */
void start_world(Game &game, World &world, int slot)
{
    game.current_slot = slot;

    // loading takes a moment, so show something instead of a frozen menu
    draw_loading_screen("Loading world...");

    if (!load_world(slot, world, game.cam, game.player))
    {
        draw_loading_screen("Generating world...");

        build_world(world, game.save_slots.slots[slot].settings);

        game.cam.position = find_spawn(world);
        game.cam.yaw = 0.0;
        game.cam.pitch = 0.0;
        game.player.is_flying = false;

        save_current_world(game, world);
    }

    // not saved, so these start the same whether the world was loaded or built
    game.player.velocity_y = 0.0;
    game.player.on_ground = false;
    game.is_camera_detached = false;

    // throws away any mouse movements from before to stop mouse movement on the first frame
    lock_cursor(game.is_cursor_locked);

    // loading took a while, so time the next frame from now not from before it
    game.last_frame_ms = time_now_ms();

    game.screen = SCREEN_PLAYING;
}

// ============================================================
// MENU SCREENS
// each one draws itself first, then acts on what was clicked.
// the click was already read by process_events, so the order doesn't change what happens
// ============================================================

void update_main_menu(Game &game)
{
    MainMenu menu = make_main_menu();
    draw_main_menu(menu);
    refresh_screen(chosen_fps_cap(game.settings));

    MenuChoice choice = main_menu_choice(menu);

    if (choice == MENU_PLAY)
    {
        game.screen = SCREEN_WORLD_SELECT;
    }
    else if (choice == MENU_SETTINGS)
    {
        game.settings_return_screen = SCREEN_MAIN_MENU;
        game.screen = SCREEN_SETTINGS;
    }
    else if (choice == MENU_EXIT)
    {
        game.should_quit = true;
    }
}

void update_world_select(Game &game, World &world)
{
    WorldSelectMenu menu = make_world_select_menu(game.save_slots);
    draw_world_select_menu(menu);
    refresh_screen(chosen_fps_cap(game.settings));

    WorldSelectChoice choice = world_select_choice(menu);

    if (choice.action == SELECT_SLOT)
    {
        if (game.save_slots.slots[choice.slot].is_used)
        {
            start_world(game, world, choice.slot);
        }
        else
        {
            // empty slot, so make a new world in it
            game.create_form = make_create_world_form(choice.slot);
            game.screen = SCREEN_CREATE_WORLD;
        }
    }
    else if (choice.action == SELECT_DELETE)
    {
        // ask first, nothing is deleted until it's confirmed
        game.delete_slot = choice.slot;
        game.screen = SCREEN_DELETE_WORLD;
    }
    else if (choice.action == SELECT_BACK)
    {
        game.screen = SCREEN_MAIN_MENU;
    }
}

void update_create_world(Game &game, World &world)
{
    CreateWorldMenu menu = make_create_world_menu(game.create_form);
    draw_create_world_menu(menu, game.create_form);
    refresh_screen(chosen_fps_cap(game.settings));

    CreateChoice choice = handle_create_world_menu(menu, game.create_form);

    if (choice == CREATE_CREATE)
    {
        // the slot is used from now on, and shows the world's name on the select screen
        int slot = game.create_form.slot;
        game.save_slots.slots[slot].settings = settings_from_form(game.create_form);
        game.save_slots.slots[slot].is_used = true;

        start_world(game, world, slot);
    }
    else if (choice == CREATE_CANCEL)
    {
        // the slot stays empty
        game.screen = SCREEN_WORLD_SELECT;
    }
}

void update_delete_world(Game &game)
{
    DeleteMenu menu = make_delete_menu();
    draw_delete_menu(menu, game.save_slots.slots[game.delete_slot].settings.name);
    refresh_screen(chosen_fps_cap(game.settings));

    DeleteChoice choice = delete_menu_choice(menu);

    if (choice == DELETE_CONFIRM)
    {
        delete_world(game.delete_slot);

        // shows as an empty slot straight away, without reading the files again
        game.save_slots.slots[game.delete_slot].is_used = false;
        game.screen = SCREEN_WORLD_SELECT;
    }
    else if (choice == DELETE_CANCEL)
    {
        game.screen = SCREEN_WORLD_SELECT;
    }
}

// nothing in the world runs here, even when it was opened from the pause menu
void update_settings(Game &game)
{
    SettingsMenu menu = make_settings_menu(game.settings, game.settings_page);
    draw_settings_menu(menu);
    refresh_screen(chosen_fps_cap(game.settings));

    if (handle_settings_menu(menu, game.settings, game.settings_page))
    {
        // saved once when leaving, instead of on every click
        save_settings(game.settings);
        game.screen = game.settings_return_screen;
    }
}

// ============================================================
// PLAYING
// ============================================================

/**
 * one frame of actually playing, only while the cursor is locked.
 * only one camera is controlled at a time: while detached the player is frozen,
 * no movement, no gravity, no breaking or placing
 */
void update_gameplay(Game &game, World &world, double dt)
{
    // time only passes while playing, so pausing stops the sun too
    advance_time(game.time_of_day, chosen_day_length(game.settings), dt);

    if (game.is_camera_detached)
    {
        handle_free_fly(game.debug_cam, dt);
        handle_mouse_look(game.debug_cam, chosen_sensitivity(game.settings));
    }
    else
    {
        handle_movement(game.cam, game.player, world, dt);
        handle_mouse_look(game.cam, chosen_sensitivity(game.settings));

        // after mouse look, so the ray uses where you're looking this frame
        handle_block_break(game.cam, world);
        handle_block_place(game.cam, world, game.selected_block);
    }
}

// the pause menu's buttons, while the cursor is unlocked
void handle_pause_menu(Game &game, const World &world)
{
    PauseChoice choice = pause_menu_choice(make_pause_menu());

    if (choice == PAUSE_RESUME)
    {
        lock_cursor(game.is_cursor_locked);
    }
    else if (choice == PAUSE_SETTINGS)
    {
        // back to the pause menu after, the cursor stays unlocked
        game.settings_return_screen = SCREEN_PLAYING;
        game.screen = SCREEN_SETTINGS;
    }
    else if (choice == PAUSE_QUIT_TO_TITLE)
    {
        draw_loading_screen("Saving world...");
        save_current_world(game, world);

        // the cursor is already unlocked, which the main menu needs
        game.screen = SCREEN_MAIN_MENU;
    }
}

/**
 * draws one frame of the world, back to front:
 * the sky, the blocks, then everything that goes on top of them
 */
void draw_game(Game &game, const World &world)
{
    // settings can change while paused, so the field of view is taken from them every frame
    game.cam.fov = chosen_fov(game.settings);
    game.debug_cam.fov = game.cam.fov;

    // draw from whichever camera is in control, a camera is only a few numbers so copying it every frame is ok
    Camera view_cam = game.cam;
    if (game.is_camera_detached)
    {
        view_cam = game.debug_cam;
    }

    // drawn first: it clears the screen, and the sun and moon end up behind the world
    draw_sky(view_cam, game.time_of_day);

    int faces_drawn = draw_world_solid(view_cam, game.cam, world, game.see_through, game.time_of_day, game.settings, game.frame_timer);

    // after the world so it covers all of it, before the crosshair and hud
    draw_underwater_tint(view_cam, world);

    // only from the debug camera, from the player's own eye the lines would just be the screen's edges
    if (game.is_camera_detached && is_on(game.settings, SETTING_SHOW_FRUSTUM))
    {
        draw_player_frustum(view_cam, game.cam);
    }

    draw_crosshair();
    draw_selected_block(game.selected_block);

    // shows which camera you're looking through, so the frozen player isn't mistaken for a bug
    if (game.is_camera_detached)
    {
        draw_debug_cam_label();
    }

    // over the world and hud, under the fps counter
    if (!game.is_cursor_locked)
    {
        draw_pause_menu(make_pause_menu());
    }

    // last, so it's drawn on top of everything
    update_fps_counter(game.fps_counter);
    if (is_on(game.settings, SETTING_SHOW_FPS))
    {
        draw_fps_counter(game.fps_counter);
    }

    // the player's camera, so it shows where the player is even from the debug camera
    if (is_on(game.settings, SETTING_DEBUG_INFO))
    {
        draw_debug_info(game.cam, faces_drawn, game.time_of_day);
    }

    double present_start = time_now_ms();
    refresh_screen(chosen_fps_cap(game.settings));
    add_stage_time(game.frame_timer, STAGE_PRESENT, present_start);

    end_timed_frame(game.frame_timer);
}

/**
 * one frame in a world: the toggle keys, then either playing or the pause menu, then drawing.
 * escape unlocks the cursor and pauses: nothing moves, no gravity,
 * no breaking or placing until the cursor is locked again
 */
void update_playing(Game &game, World &world, double dt)
{
    handle_cursor_toggle(game.is_cursor_locked);
    handle_fly_toggle(game.player);
    handle_see_through_toggle(game.see_through);
    handle_camera_detach_toggle(game.is_camera_detached, game.debug_cam, game.cam);
    handle_block_select(game.selected_block);

    if (game.is_cursor_locked)
    {
        update_gameplay(game, world, dt);
    }
    else
    {
        handle_pause_menu(game, world);
    }

    // the pause menu can leave the world (settings, quit to title), then there's nothing to draw
    if (game.screen != SCREEN_PLAYING)
    {
        return;
    }

    draw_game(game, world);
}
