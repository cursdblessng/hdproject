#include "splashkit.h"
#include "world.h"
#include "render.h"
#include "input.h"

int main()
{
    open_window("poopcraft", 800, 600);

    Camera cam;
    // Moved camera up slightly so we can see the grid below us
    cam.position = {0, 2, -5};
    cam.yaw = 0.0;
    cam.pitch = 0.0;
    cam.fov = 1.0;

    BlockArray world;
    init_block_array(world);
    build_test_world(world);

    bool is_cursor_locked = true;

    hide_mouse();
    move_mouse(screen_width() / 2.0, screen_height() / 2.0);

    double mouse_sensitivity = 0.003;

    while (!quit_requested())
    {
        process_events();

        // inputs
        handle_cursor_toggle(is_cursor_locked);
        handle_movement(cam);

        if (is_cursor_locked)
        {
            handle_mouse_look(cam, mouse_sensitivity);
        }

        // drawing
        clear_screen(COLOR_SKY_BLUE);

        draw_grid(cam, 20);
        draw_world_solid(cam, world);

        // crosshair
        fill_circle(COLOR_WHITE, screen_width() / 2.0, screen_height() / 2.0, 2);

        refresh_screen(60);
    }

    close_window("poopcraft");
    return 0;
}
