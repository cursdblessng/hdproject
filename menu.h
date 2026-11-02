// main menu: title, buttons, and which screen the game is on
#pragma once

#include "splashkit.h"
#include "world.h"
#include "input.h"
#include "settings.h"

// which screen main is showing, decides what each frame does
enum GameScreen
{
    SCREEN_MAIN_MENU,
    SCREEN_WORLD_SELECT,
    SCREEN_CREATE_WORLD,
    SCREEN_DELETE_WORLD,
    SCREEN_SETTINGS,
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

// what was clicked on the pause menu this frame, PAUSE_NONE if nothing
enum PauseChoice
{
    PAUSE_NONE,
    PAUSE_RESUME,
    PAUSE_SETTINGS,
    PAUSE_QUIT_TO_TITLE
};

// every button on the pause menu
struct PauseMenu
{
    Button resume;
    Button settings;
    Button quit_to_title;
};

// buttons laid out for the current window size
PauseMenu make_pause_menu();

// which button was clicked this frame
PauseChoice pause_menu_choice(const PauseMenu &menu);

// darkens the world and draws the title and buttons over it
void draw_pause_menu(const PauseMenu &menu);

// loads the menu font, call once after opening the window
void load_menu_font();

// buttons laid out for the current window size
MainMenu make_main_menu();

// which button was clicked this frame
MenuChoice main_menu_choice(const MainMenu &menu);

// title and buttons over the whole screen
void draw_main_menu(const MainMenu &menu);

// shown while the world builds, so the game doesn't look frozen
void draw_loading_screen(const string &message);

// what was clicked on world select screen
enum WorldSelectAction
{
    SELECT_NONE,
    SELECT_SLOT,
    SELECT_DELETE,
    SELECT_BACK
};

// the action, plus which slot it was for SELECT_SLOT and SELECT_DELETE
struct WorldSelectChoice
{
    WorldSelectAction action;
    int slot;
};

// one button per save slot, a delete button next to each used one, then back
struct WorldSelectMenu
{
    Button slots[NUM_SAVE_SLOTS];
    Button deletes[NUM_SAVE_SLOTS];
    bool can_delete[NUM_SAVE_SLOTS]; // only used slots have a world to delete
    Button back;
};

// buttons laid out for the current window size, labelled from the slots
WorldSelectMenu make_world_select_menu(const SaveSlots &save_slots);

// which button was clicked this frame
WorldSelectChoice world_select_choice(const WorldSelectMenu &menu);

// title and buttons over the whole screen
void draw_world_select_menu(const WorldSelectMenu &menu);

// which text box is being typed in
enum TextField
{
    FIELD_NONE,
    FIELD_NAME,
    FIELD_SEED
};

// what's been entered on the create world screen so far.
// kept in main because typing carries on over many frames
struct CreateWorldForm
{
    int slot;               // which save slot the new world goes in
    string name;            // blank = "World" and the slot number
    string seed_text;       // blank = random seed
    bool is_superflat;
    TextField active_field; // which box is being typed in, FIELD_NONE if neither
};

// what was clicked on the create world screen
enum CreateChoice
{
    CREATE_NONE,
    CREATE_CREATE,
    CREATE_CANCEL
};

// every box and button on the create world screen.
// the text boxes are Buttons so they're clicked the same way, their label is the typed text
struct CreateWorldMenu
{
    Button name_box;
    Button seed_box;
    Button type;
    Button create;
    Button cancel;
};

// a blank form for a new world in this slot
CreateWorldForm make_create_world_form(int slot);

// boxes and buttons laid out for the current window size, labelled from the form
CreateWorldMenu make_create_world_menu(const CreateWorldForm &form);

// the settings a finished form makes, blank boxes filled with their defaults
WorldSettings settings_from_form(const CreateWorldForm &form);

// typing, clicking boxes and the type button. returns create or cancel when clicked
CreateChoice handle_create_world_menu(const CreateWorldMenu &menu, CreateWorldForm &form);

// title, labels, boxes and buttons over the whole screen
void draw_create_world_menu(const CreateWorldMenu &menu, const CreateWorldForm &form);

// loads the slot's world, or builds and saves a new one if it has no save
void start_world(int slot, const WorldSettings &settings, World &world, Camera &cam, Player &player, bool &is_camera_detached, bool &is_cursor_locked);

// what was clicked on the delete world screen
enum DeleteChoice
{
    DELETE_NONE,
    DELETE_CONFIRM,
    DELETE_CANCEL
};

// asks before deleting, because a deleted world can't be brought back
struct DeleteMenu
{
    Button confirm;
    Button cancel;
};

// buttons laid out for the current window size
DeleteMenu make_delete_menu();

// which button was clicked this frame
DeleteChoice delete_menu_choice(const DeleteMenu &menu);

// title, the world's name and the buttons over the whole screen
void draw_delete_menu(const DeleteMenu &menu, const string &world_name);

// red dot and "DEBUG CAMERA" in the bottom right corner, drawn while the camera is detached
void draw_debug_cam_label();
// one button per setting showing its current option, then done and a button to switch page.
// only the settings on the current page are laid out, drawn and clicked
struct SettingsMenu
{
    Button options[NUM_SETTINGS];
    Button done;
    Button page_switch;
    int page;
};

// buttons for one page laid out for the current window size, labelled from the settings
SettingsMenu make_settings_menu(const Settings &settings, int page);

// left click moves a setting to its next option, right click back, the page button changes page.
// true when done is clicked
bool handle_settings_menu(const SettingsMenu &menu, Settings &settings, int &page);

// title, each setting's name next to its button, and done, over the whole screen
void draw_settings_menu(const SettingsMenu &menu);
