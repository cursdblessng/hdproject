// main menu: title, buttons, and which screen the game is on
#pragma once

#include "splashkit.h"

// which screen main is showing, decides what each frame does
enum GameScreen
{
    SCREEN_MAIN_MENU,
    SCREEN_PLAYING
};

// what was selected by the user
enum MenuChoice
{
    MENU_NONE,
    MENU_PLAY,
    MENU_SETTINGS,
    MENU_EXIT
};

// a clickable box with text in the middle
struct Button
{
    rectangle area; // position and size in pixels
    string label;
};

// all main menu buttons
struct MainMenu
{
    Button play;
    Button settings;
    Button exit;
};

// loads the menu font, call once after opening the window
void load_menu_font();

// buttons laid out for the current window size
MainMenu make_main_menu();

// which button was clicked this frame
MenuChoice main_menu_choice(const MainMenu &menu);

// title and buttons over the whole screen
void draw_main_menu(const MainMenu &menu);

// shown while the world builds, so the game doesn't look frozen
void draw_loading_screen();