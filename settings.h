// player settings: what each one is called, its options, and the values the game uses
#pragma once

#include "splashkit.h"

/**
 * every setting, used as the index into Settings.choice and the tables in settings.cpp.
 * NUM_SETTINGS is last, so it's always equal to how many settings come before it
 */
enum SettingId
{
    SETTING_SENSITIVITY,
    SETTING_FOV,
    SETTING_FPS_CAP,
    SETTING_DAY_LENGTH,
    SETTING_SHOW_FPS,
    SETTING_FRUSTUM_CULLING,
    SETTING_BACK_FACE_CULLING,
    SETTING_HIDDEN_FACE_CULLING,
    SETTING_OUTLINES,
    SETTING_DEBUG_INFO,
    SETTING_SHOW_FRUSTUM,
    NUM_SETTINGS
};

// settings are split into pages so they fit on the screen: 0 = game, 1 = debug
const int NUM_SETTING_PAGES = 2;

// which option is picked for each setting, e.g. choice[SETTING_FPS_CAP] = 1 means the 2nd option (60)
struct Settings
{
    int choice[NUM_SETTINGS];
};

// every setting on its starting option
Settings default_settings();

// what the setting is called on the settings screen
string setting_name(int setting);

// which page the setting is on
int setting_page(int setting);

// the title shown at the top of a page
string page_title(int page);

// the short name of a page, for the button that switches to it
string page_name(int page);

// what the setting's current option shows on its button
string option_label(const Settings &settings, int setting);

// moves a setting on to its next option, back to the first after the last
void next_option(Settings &settings, int setting);

// moves a setting back to its previous option, round to the last before the first
void previous_option(Settings &settings, int setting);

// does this setting have an option with this number, used to check a loaded settings file
bool is_valid_option(int setting, int option);

// the values behind the chosen options, what the rest of the game actually uses
double chosen_sensitivity(const Settings &settings);
double chosen_fov(const Settings &settings); // radians
unsigned int chosen_fps_cap(const Settings &settings); // 0 = no cap
double chosen_day_length(const Settings &settings); // seconds

// for the on/off settings, true when it's on
bool is_on(const Settings &settings, int setting);
