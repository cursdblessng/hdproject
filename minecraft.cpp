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

    // starts on foot, press f to fly
    Player player;
    player.velocity_y = 0.0;
    player.on_ground = false; // spawns in the air
    player.is_flying = false;

    // which block right click places, changed with keys 1-6
    BlockType selected_block = GRASS;

    hide_mouse();
    move_mouse(screen_width() / 2.0, screen_height() / 2.0);

    double mouse_sensitivity = 0.003;

    while (!quit_requested())
    {
        process_events();

        // inputs
        handle_cursor_toggle(is_cursor_locked);
        handle_fly_toggle(player);
        handle_block_select(selected_block);
        handle_movement(cam, player, world);

        if (is_cursor_locked)
        {
            handle_mouse_look(cam, mouse_sensitivity);

            // after mouse look, so the ray uses where you're looking THIS frame
            handle_block_break(cam, world);
            handle_block_place(cam, world, selected_block);
        }

        // drawing
        clear_screen(COLOR_SKY_BLUE);

        draw_grid(cam, 20);
        draw_world_solid(cam, world);

        // crosshair
        fill_circle(COLOR_WHITE, screen_width() / 2.0, screen_height() / 2.0, 2);

        // TEMPORARY: small square in the bottom left showing the selected block's
        // colour, so you can tell what you're holding. a proper hotbar comes later
        fill_rectangle(block_colour(selected_block), 10, screen_height() - 40, 30, 30);

        refresh_screen(60);
    }

    close_window("poopcraft");
    return 0;
}