#include "splashkit.h"
#include "world.h"
#include "render.h"
#include "input.h"

int main()
{
    open_window("poopcraft", WINDOWED_WIDTH, WINDOWED_HEIGHT);

    // the entire world
    // not stored on the 1mb stack
    static World world;

    // which world to make. constants for now, later the menu will choose
    // the type and the seed, and saving will store them
    const bool USE_SUPERFLAT = false;
    const int SUPERFLAT_SIZE = 100;
    const unsigned int WORLD_SEED = 12345;

    if (USE_SUPERFLAT)
    {
        build_superflat(world, SUPERFLAT_SIZE);
    }
    else
    {
        build_terrain(world, WORLD_SEED);
    }

    Camera cam;

    int spawn_ground = surface_y(world, 256, 256);
    cam.position = {256.5, spawn_ground + 2.0, 256.5};
    cam.yaw = 0.0;
    cam.pitch = 0.0;
    cam.fov = 2.0;

    bool is_cursor_locked = true;

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

    hide_mouse();
    move_mouse(screen_width() / 2.0, screen_height() / 2.0);

    double mouse_sensitivity = 0.002;

    while (!quit_requested())
    {
        process_events();

        // inputs
        handle_cursor_toggle(is_cursor_locked);
        handle_fullscreen_toggle(is_cursor_locked);
        handle_fly_toggle(player);
        handle_see_through_toggle(see_through);
        handle_camera_detach_toggle(is_camera_detached, debug_cam, cam);
        handle_block_select(selected_block);

        // only one camera is controlled at a time. while detached the
        // player is frozen: no movement, no gravity, no breaking or placing
        if (is_camera_detached)
        {
            handle_free_fly(debug_cam);
        }
        else
        {
            handle_movement(cam, player, world);
        }

        if (is_cursor_locked)
        {
            if (is_camera_detached)
            {
                handle_mouse_look(debug_cam, mouse_sensitivity);
            }
            else
            {
                handle_mouse_look(cam, mouse_sensitivity);

                // after mouse look, so the ray uses where you're looking THIS frame
                handle_block_break(cam, world);
                handle_block_place(cam, world, selected_block);
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

        draw_grid(view_cam, 20);
        draw_world_solid(view_cam, cam, world, see_through);

        // crosshair
        fill_circle(COLOR_WHITE, screen_width() / 2.0, screen_height() / 2.0, 2);

        // TEMPORARY: small square in the bottom left showing the selected block's
        // colour, so you can tell what you're holding. a proper hotbar comes later
        fill_rectangle(block_colour(selected_block), 10, screen_height() - 40, 30, 30);

        // last, so it's drawn on top of everything
        update_fps_counter(fps_counter);
        draw_fps_counter(fps_counter);

        refresh_screen(TARGET_FPS);
    }

    close_window("poopcraft");
    return 0;
}