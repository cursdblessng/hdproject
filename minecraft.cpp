#include "splashkit.h"
#include "world.h"
#include "render.h"
#include "input.h"
#include "menu.h"
#include "save.h"
#include "settings.h"

int main()
{
    open_window("Voxel Engine", WINDOWED_WIDTH, WINDOWED_HEIGHT);
    load_menu_font();

    // the entire world, 32 mb
    // static so it isn't stored on the 1 mb stack
    static World world;

    // the player's settings, read from settings.txt or the defaults if there isn't one
    Settings settings = load_settings();

    // where done on the settings screen goes back to, the main menu or the paused game
    GameScreen settings_return_screen = SCREEN_MAIN_MENU;

    // which page of settings is showing, stays on the last one used
    int settings_page = 0;

    // the worlds you can pick on the world select screen, read from the save files
    SaveSlots save_slots = load_save_slots();

    // which slot is being played, so quitting knows where to save. set when a world is started
    int current_slot = 0;

    // opens on the main menu, the world is loaded or built when a slot is picked
    GameScreen screen = SCREEN_MAIN_MENU;

    // what has been typed and picked on the create world screen, reset when an empty slot is clicked
    CreateWorldForm create_form = make_create_world_form(0);

    // how far through the day it is, from 0 to 1: 0 = sunrise, 0.25 = noon, 0.5 = sunset, 0.75 = midnight.
    // starts in the morning so the first world opens in daylight
    double time_of_day = 0.1;

    // which slot the delete world screen is asking about, set when its delete button is clicked
    int delete_slot = 0;

    Camera cam;

    cam.position = {0.0, 0.0, 0.0}; // moved to the spawn when a slot is picked
    cam.yaw = 0.0;
    cam.pitch = 0.0;
    cam.fov = chosen_fov(settings);

    // unlocked so the mouse can click the menu
    bool is_cursor_locked = false;

    // starts on foot, press f to fly
    Player player;
    player.velocity_y = 0.0;
    player.on_ground = false; // spawns in the air
    player.is_flying = false;

    // which block right click places, changed with keys 1-7
    BlockType selected_block = GRASS;

    // debug view: half transparent faces, toggled with x
    bool see_through = false;

    // debug camera: c detaches the view from the player so you can fly
    // around and look at the world from outside. the player stays frozen
    bool is_camera_detached = false;
    Camera debug_cam = cam; // overwritten on detach, set here so it's never garbage

    // measures frames per second, in render.cpp
    FpsCounter fps_counter;
    fps_counter.last_update_ticks = current_ticks();
    fps_counter.frames_since_update = 0;
    fps_counter.fps = 0.0;

    // times each part of the frame and prints the averages every second, in timing.cpp
    FrameTimer frame_timer = make_frame_timer();

    // a frame longer than this (dragging the window, a big hitch) is treated as this long,
    // so the player doesn't suddenly jump a long way after the game stalls
    const double MAX_FRAME_SECONDS = 0.25;

    // when the last frame started, so each frame knows how long the one before took
    double last_frame_ms = time_now_ms();

    while (!quit_requested())
    {
        start_timed_frame(frame_timer);

        // seconds since the last frame, used to make movement per second instead of per frame
        double now_ms = time_now_ms();
        double dt = (now_ms - last_frame_ms) / 1000.0;
        last_frame_ms = now_ms;

        if (dt > MAX_FRAME_SECONDS)
        {
            dt = MAX_FRAME_SECONDS;
        }

        process_events();

        // before the menu so f11 works on every screen
        handle_fullscreen_toggle(is_cursor_locked);

        if (screen == SCREEN_MAIN_MENU)
        {
            MainMenu menu = make_main_menu();
            draw_main_menu(menu);
            refresh_screen(chosen_fps_cap(settings));

            MenuChoice choice = main_menu_choice(menu);

            if (choice == MENU_PLAY)
            {
                screen = SCREEN_WORLD_SELECT;
            }
            else if (choice == MENU_EXIT)
            {
                break;
            }
            else if (choice == MENU_SETTINGS)
            {
                settings_return_screen = SCREEN_MAIN_MENU;
                screen = SCREEN_SETTINGS;
            }

            // skip the game part of the loop, nothing in the world runs on the menu
            continue;
        }

        if (screen == SCREEN_WORLD_SELECT)
        {
            WorldSelectMenu select_menu = make_world_select_menu(save_slots);
            draw_world_select_menu(select_menu);
            refresh_screen(chosen_fps_cap(settings));

            WorldSelectChoice select_choice = world_select_choice(select_menu);

            if (select_choice.action == SELECT_SLOT)
            {
                int slot = select_choice.slot;

                if (save_slots.slots[slot].is_used)
                {
                    start_world(slot, save_slots.slots[slot].settings, world, cam, player, is_camera_detached, is_cursor_locked);
                    current_slot = slot;

                    // loading took a while, so time the next frame from now not from before it
                    last_frame_ms = time_now_ms();

                    screen = SCREEN_PLAYING;
                }
                else
                {
                    // empty slot, so make a new world in it
                    create_form = make_create_world_form(slot);
                    screen = SCREEN_CREATE_WORLD;
                }
            }
            else if (select_choice.action == SELECT_DELETE)
            {
                // ask first, nothing is deleted until it's confirmed
                delete_slot = select_choice.slot;
                screen = SCREEN_DELETE_WORLD;
            }
            else if (select_choice.action == SELECT_BACK)
            {
                screen = SCREEN_MAIN_MENU;
            }

            // same as the main menu, nothing in the world runs here
            continue;
        }

        if (screen == SCREEN_CREATE_WORLD)
        {
            CreateWorldMenu create_menu = make_create_world_menu(create_form);
            draw_create_world_menu(create_menu, create_form);
            refresh_screen(chosen_fps_cap(settings));

            CreateChoice create_choice = handle_create_world_menu(create_menu, create_form);

            if (create_choice == CREATE_CREATE)
            {
                // the slot is used from now on, and shows the world's name on the select screen
                int slot = create_form.slot;
                save_slots.slots[slot].settings = settings_from_form(create_form);
                save_slots.slots[slot].is_used = true;

                start_world(slot, save_slots.slots[slot].settings, world, cam, player, is_camera_detached, is_cursor_locked);
                current_slot = slot;

                // building took a while, so time the next frame from now not from before it
                last_frame_ms = time_now_ms();

                screen = SCREEN_PLAYING;
            }
            else if (create_choice == CREATE_CANCEL)
            {
                // the slot stays empty
                screen = SCREEN_WORLD_SELECT;
            }

            // same as the main menu, nothing in the world runs here
            continue;
        }

        if (screen == SCREEN_DELETE_WORLD)
        {
            DeleteMenu delete_menu = make_delete_menu();
            draw_delete_menu(delete_menu, save_slots.slots[delete_slot].settings.name);
            refresh_screen(chosen_fps_cap(settings));

            DeleteChoice delete_choice = delete_menu_choice(delete_menu);

            if (delete_choice == DELETE_CONFIRM)
            {
                delete_world(delete_slot);

                // shows as an empty slot straight away, without reading the files again
                save_slots.slots[delete_slot].is_used = false;
                screen = SCREEN_WORLD_SELECT;
            }
            else if (delete_choice == DELETE_CANCEL)
            {
                screen = SCREEN_WORLD_SELECT;
            }

            // same as the main menu, nothing in the world runs here
            continue;
        }

        if (screen == SCREEN_SETTINGS)
        {
            SettingsMenu settings_menu = make_settings_menu(settings, settings_page);
            draw_settings_menu(settings_menu);
            refresh_screen(chosen_fps_cap(settings));

            if (handle_settings_menu(settings_menu, settings, settings_page))
            {
                // saved once when leaving, instead of on every click
                save_settings(settings);
                screen = settings_return_screen;
            }

            // nothing in the world runs here, even when it was opened from the pause menu
            continue;
        }

        // inputs
        handle_cursor_toggle(is_cursor_locked);
        handle_fly_toggle(player);
        handle_see_through_toggle(see_through);
        handle_camera_detach_toggle(is_camera_detached, debug_cam, cam);
        handle_block_select(selected_block);

        PauseMenu pause_menu = make_pause_menu();

        // escape unlocks the cursor and pauses: nothing moves, no gravity,
        // no breaking or placing until the cursor is locked again.
        // only one camera is controlled at a time. while detached the
        // player is frozen: no movement, no gravity, no breaking or placing
        if (is_cursor_locked)
        {
            // time only passes while playing, so pausing stops the sun too
            advance_time(time_of_day, chosen_day_length(settings), dt);

            if (is_camera_detached)
            {
                handle_free_fly(debug_cam, dt);
                handle_mouse_look(debug_cam, chosen_sensitivity(settings));
            }
            else
            {
                handle_movement(cam, player, world, dt);
                handle_mouse_look(cam, chosen_sensitivity(settings));

                // after mouse look, so the ray uses where you're looking this frame
                handle_block_break(cam, world);
                handle_block_place(cam, world, selected_block);
            }
        }
        else
        {
            // paused, the mouse is for the pause menu's buttons
            PauseChoice pause_choice = pause_menu_choice(pause_menu);

            if (pause_choice == PAUSE_RESUME)
            {
                lock_cursor(is_cursor_locked);
            }
            else if (pause_choice == PAUSE_SETTINGS)
            {
                // back to the pause menu after, the cursor stays unlocked
                settings_return_screen = SCREEN_PLAYING;
                screen = SCREEN_SETTINGS;
                continue;
            }
            else if (pause_choice == PAUSE_QUIT_TO_TITLE)
            {
                draw_loading_screen("Saving world...");
                save_world(current_slot, save_slots.slots[current_slot].settings, world, cam, player);

                // the cursor is already unlocked, which the main menu needs
                screen = SCREEN_MAIN_MENU;
                continue;
            }
        }

        // drawing
        // settings can change while paused, so the field of view is taken from them every frame
        cam.fov = chosen_fov(settings);
        debug_cam.fov = cam.fov;

        // draw from whichever camera is in control, a camera is only a few numbers so calculating it every frame is ok
        Camera view_cam = cam;
        if (is_camera_detached)
        {
            view_cam = debug_cam;
        }

        // drawn first: it clears the screen, and the sun and moon end up behind the world
        draw_sky(view_cam, time_of_day);

        int faces_drawn = draw_world_solid(view_cam, cam, world, see_through, time_of_day, settings, frame_timer);

        // after the world so it covers all of it, before the crosshair and hud
        draw_underwater_tint(view_cam, world);

        // only from the debug camera, from the player's own eye the lines would just be the screen's edges
        if (is_camera_detached && is_on(settings, SETTING_SHOW_FRUSTUM))
        {
            draw_player_frustum(view_cam, cam);
        }

        // crosshair
        fill_circle(COLOR_WHITE, screen_width() / 2.0, screen_height() / 2.0, 2);

        // small square in the bottom left showing the selected block's colour, so you can tell what you're holding.
        // a black square 2 pixels bigger on every side goes underneath first,
        // so the edge left showing is a border that stands out on any background
        fill_rectangle(COLOR_BLACK, 8, screen_height() - 42, 34, 34);
        fill_rectangle(block_colour(selected_block), 10, screen_height() - 40, 30, 30);

        // shows which camera you're looking through, so the frozen player isn't mistaken for a bug
        if (is_camera_detached)
        {
            draw_debug_cam_label();
        }

        // over the world and hud, under the fps counter
        if (!is_cursor_locked)
        {
            draw_pause_menu(pause_menu);
        }

        // last, so it's drawn on top of everything
        update_fps_counter(fps_counter);
        if (is_on(settings, SETTING_SHOW_FPS))
        {
            draw_fps_counter(fps_counter);
        }

        // the player's camera, so it shows where the player is even from the debug camera
        if (is_on(settings, SETTING_DEBUG_INFO))
        {
            draw_debug_info(cam, faces_drawn, time_of_day);
        }

        double present_start = time_now_ms();
        refresh_screen(chosen_fps_cap(settings));
        add_stage_time(frame_timer, STAGE_PRESENT, present_start);

        end_timed_frame(frame_timer);
    }

    // closing the window while playing saves too, so nothing is lost
    if (screen == SCREEN_PLAYING)
    {
        save_world(current_slot, save_slots.slots[current_slot].settings, world, cam, player);
    }

    close_window("Voxel Engine");
    return 0;
}