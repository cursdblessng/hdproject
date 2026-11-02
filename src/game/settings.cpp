#include "settings.h"

const double PI = 3.14159265358979323846;

// the most options any one setting has, so the label table can be a normal 2d array
const int MAX_OPTIONS = 5;

// every table below is in SettingId order, one row per setting

// what each setting is called on the settings screen
const string SETTING_NAMES[NUM_SETTINGS] = {"Sensitivity", "Field of View", "FPS Cap", "Day Length", "Show FPS",
                                             "Frustum Cull", "Back Face Cull", "Hidden Face Cull", "Block Outlines", "Debug Info", "Player Frustum"};

// which page each setting is on, the game ones first then the debug ones
const int SETTING_PAGES[NUM_SETTINGS] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1};

// how many options each setting has
const int OPTION_COUNTS[NUM_SETTINGS] = {5, 5, 4, 4, 2, 2, 2, 2, 2, 2, 2};

// what each option shows on its button. settings with fewer options leave the rest blank
// AI assisted
const string OPTION_LABELS[NUM_SETTINGS][MAX_OPTIONS] = {
    {"Very Low", "Low", "Normal", "High", "Very High"},
    {"70", "80", "90", "100", "115"},
    {"30", "60", "144", "Unlimited", ""},
    {"1 min", "5 min", "10 min", "20 min", ""},
    {"Off", "On", "", "", ""},
    {"Off", "On", "", "", ""},
    {"Off", "On", "", "", ""},
    {"Off", "On", "", "", ""},
    {"Off", "On", "", "", ""},
    {"Off", "On", "", "", ""},
    {"Off", "On", "", "", ""}};

// the option each setting starts on: normal sensitivity, 115 fov, 60 fps, 1 minute days, fps shown,
// all culling on, no outlines, no debug info, player frustum shown in the debug camera
const int DEFAULT_OPTIONS[NUM_SETTINGS] = {2, 4, 1, 0, 1, 1, 1, 1, 0, 0, 1};

// the title at the top of each page, and the short name on the button that switches to it
const string PAGE_TITLES[NUM_SETTING_PAGES] = {"Settings", "Debug Settings"};
const string PAGE_NAMES[NUM_SETTING_PAGES] = {"Game", "Debug"};

// the values behind each option, in the same order as its labels
const double SENSITIVITY_VALUES[] = {0.001, 0.0015, 0.002, 0.003, 0.004};
const double FOV_DEGREES[] = {70.0, 80.0, 90.0, 100.0, 115.0};
const unsigned int FPS_CAP_VALUES[] = {30, 60, 144, 0}; // 0 tells refresh_screen not to wait at all
const double DAY_LENGTH_VALUES[] = {60.0, 300.0, 600.0, 1200.0};

Settings default_settings()
{
    Settings settings;
    for (int i = 0; i < NUM_SETTINGS; i++)
    {
        settings.choice[i] = DEFAULT_OPTIONS[i];
    }
    return settings;
}

string setting_name(int setting)
{
    return SETTING_NAMES[setting];
}

int setting_page(int setting)
{
    return SETTING_PAGES[setting];
}

string page_title(int page)
{
    return PAGE_TITLES[page];
}

string page_name(int page)
{
    return PAGE_NAMES[page];
}

string option_label(const Settings &settings, int setting)
{
    return OPTION_LABELS[setting][settings.choice[setting]];
}

// % wraps it back to 0 after the last option
void next_option(Settings &settings, int setting)
{
    settings.choice[setting] = (settings.choice[setting] + 1) % OPTION_COUNTS[setting];
}

// + the count before the % so it never goes below 0, in c++ -1 % 5 is -1, not 4
void previous_option(Settings &settings, int setting)
{
    settings.choice[setting] = (settings.choice[setting] - 1 + OPTION_COUNTS[setting]) % OPTION_COUNTS[setting];
}

bool is_valid_option(int setting, int option)
{
    return option >= 0 && option < OPTION_COUNTS[setting];
}

double chosen_sensitivity(const Settings &settings)
{
    return SENSITIVITY_VALUES[settings.choice[SETTING_SENSITIVITY]];
}

// stored in degrees because they're easier to read, the camera uses radians
double chosen_fov(const Settings &settings)
{
    return FOV_DEGREES[settings.choice[SETTING_FOV]] * PI / 180.0;
}

unsigned int chosen_fps_cap(const Settings &settings)
{
    return FPS_CAP_VALUES[settings.choice[SETTING_FPS_CAP]];
}

double chosen_day_length(const Settings &settings)
{
    return DAY_LENGTH_VALUES[settings.choice[SETTING_DAY_LENGTH]];
}

// every on/off setting has off first and on second
bool is_on(const Settings &settings, int setting)
{
    return settings.choice[setting] == 1;
}
