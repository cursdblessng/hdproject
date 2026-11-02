#include "splashkit.h"
#include "world.h"
#include "render.h"
#include "input.h"
#include "menu.h"

int main()
{
    open_window("Voxel Engine", WINDOWED_WIDTH, WINDOWED_HEIGHT);
    load_menu_font();

    // the entire world
    // not stored on the 1mb stack
    static World world;

    // the worlds you can pick on the world select screen, all empty to start
    SaveSlots save_slots;
    for (int i = 0; i < NUM_SAVE_SLOTS; i++)
    {
        save_slots.slots[i].is_used = false;
    }

    // opens on the main menu, the world is built when a slot is picked
    GameScreen screen = SCREEN_MAIN_MENU;

    Camera cam;

    cam.position = {0.0, 0.0, 0.0}; // moved to the spawn when a slot is picked
    cam.yaw = 0.0;
    cam.pitch = 0.0;
    cam.fov = 2.0;

    // unlocked so the mouse can click the menu
    bool is_cursor_locked = false;

    // starts on foot, press f to fly
    Player player;
    player.velocity_y = 0.0;
    player.on_ground = false; // spawns in the air
    player.is_flying = false;

    // which block right click places, changed with keys 1-6
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

    double mouse_sensitivity = 0.002;

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
            refresh_screen(TARGET_FPS);

            MenuChoice choice = main_menu_choice(menu);

            if (choice == MENU_PLAY)
            {
                screen = SCREEN_WORLD_SELECT;
            }
            else if (choice == MENU_EXIT)
            {
                break;
            }
            // MENU_SETTINGS does nothing yet

            // skip the game part of the loop, nothing in the world runs on the menu
            continue;
        }

        if (screen == SCREEN_WORLD_SELECT)
        {
            WorldSelectMenu select_menu = make_world_select_menu(save_slots);
            draw_world_select_menu(select_menu);
            refresh_screen(TARGET_FPS);

            WorldSelectChoice select_choice = world_select_choice(select_menu);

            if (select_choice.action == SELECT_SLOT)
            {
                int slot = select_choice.slot;

                // TEMPORARY: an empty slot gets a default world, a random seed so each slot is different.
                // the create world screen will replace this
                if (!save_slots.slots[slot].is_used)
                {
                    save_slots.slots[slot].settings.name = "World " + std::to_string(slot + 1);
                    save_slots.slots[slot].settings.seed = rnd(1000000);
                    save_slots.slots[slot].settings.is_superflat = false;
                    save_slots.slots[slot].is_used = true;
                }

                // building takes a moment, so show something instead of a frozen menu
                draw_loading_screen();

                build_world(world, save_slots.slots[slot].settings);

                // start fresh, nothing carried over from the last world
                cam.position = find_spawn(world);
                cam.yaw = 0.0;
                cam.pitch = 0.0;
                player.velocity_y = 0.0;
                player.on_ground = false;
                player.is_flying = false;
                is_camera_detached = false;

                // throws away any mouse movements from before to stop mouse movement on the first frame
                lock_cursor(is_cursor_locked);

                // building took a while, so time the next frame from now not from before it
                last_frame_ms = time_now_ms();

                screen = SCREEN_PLAYING;
            }
            else if (select_choice.action == SELECT_BACK)
            {
                screen = SCREEN_MAIN_MENU;
            }

            // same as the main menu, nothing in the world runs here
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
            if (is_camera_detached)
            {
                handle_free_fly(debug_cam, dt);
                handle_mouse_look(debug_cam, mouse_sensitivity);
            }
            else
            {
                handle_movement(cam, player, world, dt);
                handle_mouse_look(cam, mouse_sensitivity);

                // after mouse look, so the ray uses where you're looking THIS frame
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
            else if (pause_choice == PAUSE_QUIT_TO_TITLE)
            {
                // the cursor is already unlocked, which the main menu needs
                screen = SCREEN_MAIN_MENU;
                continue;
            }
        }

        // drawing
        clear_screen(COLOR_SKY_BLUE);

        // draw from whichever camera is in control, a camera is only a few numbers so calculating it every frame is ok
        Camera view_cam = cam;
        if (is_camera_detached)
        {
            view_cam = debug_cam;
        }

        draw_world_solid(view_cam, cam, world, see_through, frame_timer);

        // after the world so it covers all of it, before the crosshair and hud
        draw_underwater_tint(view_cam, world);

        // crosshair
        fill_circle(COLOR_WHITE, screen_width() / 2.0, screen_height() / 2.0, 2);

        // TEMPORARY: small square in the bottom left showing the selected block's
        // colour, so you can tell what you're holding. a proper hotbar comes later
        fill_rectangle(block_colour(selected_block), 10, screen_height() - 40, 30, 30);

        // over the world and hud, under the fps counter
        if (!is_cursor_locked)
        {
            draw_pause_menu(pause_menu);
        }

        // last, so it's drawn on top of everything
        update_fps_counter(fps_counter);
        draw_fps_counter(fps_counter);

        double present_start = time_now_ms();
        refresh_screen(TARGET_FPS);
        add_stage_time(frame_timer, STAGE_PRESENT, present_start);

        end_timed_frame(frame_timer);
    }

    close_window("Voxel Engine");
    return 0;
}