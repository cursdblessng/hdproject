#include "splashkit.h"
#include "game.h"
#include "../ui/menu.h" // load_menu_font

int main()
{
    open_window("Voxel Engine", WINDOWED_WIDTH, WINDOWED_HEIGHT);
    load_menu_font();

    // the entire world, 32 mb
    // static so it isn't stored on the 1 mb stack
    static World world;

    // everything else the game remembers between frames, starting on the main menu
    Game game = make_game();

    while (!quit_requested() && !game.should_quit)
    {
        // seconds since the last frame, so movement and time are the same at any frame rate
        double dt = start_frame(game);

        // each screen handles its own clicks and keys, then draws itself.
        // a screen moves to another one by changing game.screen, which takes effect next frame
        switch (game.screen)
        {
        case SCREEN_MAIN_MENU:
            update_main_menu(game);
            break;
        case SCREEN_WORLD_SELECT:
            update_world_select(game, world);
            break;
        case SCREEN_CREATE_WORLD:
            update_create_world(game, world);
            break;
        case SCREEN_DELETE_WORLD:
            update_delete_world(game);
            break;
        case SCREEN_SETTINGS:
            update_settings(game);
            break;
        case SCREEN_PLAYING:
            update_playing(game, world, dt);
            break;
        }
    }

    // closing the window while playing saves too, so nothing is lost
    if (game.screen == SCREEN_PLAYING)
    {
        save_current_world(game, world);
    }

    close_window("Voxel Engine");
    return 0;
}
