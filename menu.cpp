#include "menu.h"
#include "save.h"

// name the font is loaded under, used instead of keeping the font itself
const string MENU_FONT = "menu_font";

// text sizes in pixels. pixel fonts are about as wide as they are tall,
// so 48 keeps "Voxel Engine" (12 letters) inside the 800 wide window
const int TITLE_SIZE = 48;
const int BUTTON_TEXT_SIZE = 20;

// every button is the same size, stacked with a gap between them
const double BUTTON_WIDTH = 300.0;
const double BUTTON_HEIGHT = 50.0;
const double BUTTON_GAP = 20.0;

// smaller text above each box saying what it's for
const int LABEL_SIZE = 14;
const double LABEL_SPACE = 30.0;      // room left above a box for its label
const double TEXT_BOX_PADDING = 10.0; // gap between a text box's left edge and its text

// most characters a name or seed can have, so it fits in its box and on the slot button
const int MAX_TEXT_LENGTH = 12;

/**
 * splashkit looks for the file in Resources/fonts/.
 * load_font gives back a font, but splashkit also remembers it by name,
 * so the name is used everywhere instead of storing it
 */
void load_menu_font()
{
    load_font(MENU_FONT, "PressStart2P-Regular.ttf");
}

// a button centred on centre_x, with its top edge at top_y
Button make_button(const string &label, double centre_x, double top_y)
{
    Button button;
    button.area = rectangle_from(centre_x - BUTTON_WIDTH / 2.0, top_y, BUTTON_WIDTH, BUTTON_HEIGHT);
    button.label = label;
    return button;
}

/**
 * worked out from the window size every frame, so the menu stays centred
 * after f11 changes the size. buttons start halfway down, the title goes above
 */
MainMenu make_main_menu()
{
    double centre_x = screen_width() / 2.0;
    double first_y = screen_height() / 2.0;

    // top of one button to the top of the next
    double spacing = BUTTON_HEIGHT + BUTTON_GAP;

    MainMenu menu;
    menu.play = make_button("Play", centre_x, first_y);
    menu.settings = make_button("Settings", centre_x, first_y + spacing);
    menu.exit = make_button("Exit", centre_x, first_y + 2.0 * spacing);
    return menu;
}

// starts one button higher than the main menu, so the controls list still fits underneath
PauseMenu make_pause_menu()
{
    double centre_x = screen_width() / 2.0;
    double spacing = BUTTON_HEIGHT + BUTTON_GAP;
    double first_y = screen_height() / 2.0 - spacing;

    PauseMenu menu;
    menu.resume = make_button("Resume", centre_x, first_y);
    menu.settings = make_button("Settings", centre_x, first_y + spacing);
    menu.quit_to_title = make_button("Quit to Title", centre_x, first_y + 2.0 * spacing);
    return menu;
}

/**
 * used slot shows world name, empty slot says empty
 */
WorldSelectMenu make_world_select_menu(const SaveSlots &save_slots)
{
    double centre_x = screen_width() / 2.0;
    double spacing = BUTTON_HEIGHT + BUTTON_GAP;
    double first_y = screen_height() / 2.0 - spacing;

    WorldSelectMenu menu;

    for (int i = 0; i < NUM_SAVE_SLOTS; i++)
    {
        string label = "Empty Slot";
        if (save_slots.slots[i].is_used)
        {
            label = save_slots.slots[i].settings.name;
        }

        menu.slots[i] = make_button(label, centre_x, first_y + i * spacing);

        // a square just right of the slot, same height so it lines up
        menu.deletes[i].area = rectangle_from(menu.slots[i].area.x + BUTTON_WIDTH + BUTTON_GAP / 2.0, menu.slots[i].area.y, BUTTON_HEIGHT, BUTTON_HEIGHT);
        menu.deletes[i].label = "X";
        menu.can_delete[i] = save_slots.slots[i].is_used;
    }

    // under the last slot
    menu.back = make_button("Back", centre_x, first_y + NUM_SAVE_SLOTS * spacing);
    return menu;
}

// same layout as the pause menu, the text about the world goes above
DeleteMenu make_delete_menu()
{
    double centre_x = screen_width() / 2.0;
    double first_y = screen_height() / 2.0;
    double spacing = BUTTON_HEIGHT + BUTTON_GAP;

    DeleteMenu menu;
    menu.confirm = make_button("Delete", centre_x, first_y);
    menu.cancel = make_button("Cancel", centre_x, first_y + spacing);
    return menu;
}

/**
 * one row per setting, its button in the right half and its name drawn to the left.
 * all the rows plus done are centred top to bottom
 */
SettingsMenu make_settings_menu(const Settings &settings)
{
    double centre_x = screen_width() / 2.0;
    double spacing = BUTTON_HEIGHT + BUTTON_GAP;

    // NUM_SETTINGS rows and done, so half of that height above the middle
    double first_y = screen_height() / 2.0 - (NUM_SETTINGS + 1) * spacing / 2.0;

    // half a gap right of the middle, so the name and button have a gap between them
    double button_centre_x = centre_x + BUTTON_GAP / 2.0 + BUTTON_WIDTH / 2.0;

    SettingsMenu menu;
    for (int i = 0; i < NUM_SETTINGS; i++)
    {
        menu.options[i] = make_button(option_label(settings, i), button_centre_x, first_y + i * spacing);
    }

    menu.done = make_button("Done", centre_x, first_y + NUM_SETTINGS * spacing);
    return menu;
}

CreateWorldForm make_create_world_form(int slot)
{
    CreateWorldForm form;
    form.slot = slot;
    form.name = "";
    form.seed_text = "";
    form.is_superflat = false;
    form.active_field = FIELD_NONE;
    return form;
}

CreateWorldMenu make_create_world_menu(const CreateWorldForm &form)
{
    double centre_x = screen_width() / 2.0;
    double row_spacing = BUTTON_HEIGHT + BUTTON_GAP + LABEL_SPACE;

    // everything is about 300 tall so start at around 150 above the middle
    double first_y = screen_height() / 2.0 - 150.0;

    string type_label = "Normal";
    if (form.is_superflat)
    {
        type_label = "Superflat";
    }

    CreateWorldMenu menu;
    menu.name_box = make_button(form.name, centre_x, first_y);
    menu.seed_box = make_button(form.seed_text, centre_x, first_y + row_spacing);
    menu.type = make_button(type_label, centre_x, first_y + 2.0 * row_spacing);

    // the last row has no label, so it sits the normal gap under type
    double bottom_y = first_y + 3.0 * row_spacing - LABEL_SPACE;
    double half_width = (BUTTON_WIDTH - BUTTON_GAP) / 2.0;

    // make_button makes full width buttons, so these are shrunk to half and cancel is moved right
    menu.create = make_button("Create", centre_x, bottom_y);
    menu.create.area.width = half_width;

    menu.cancel = make_button("Cancel", centre_x, bottom_y);
    menu.cancel.area.width = half_width;
    menu.cancel.area.x += half_width + BUTTON_GAP;

    return menu;
}

// what a world is called if the name box is left blank, slots are 0-2 in code but 1-3 on screen
string default_world_name(int slot)
{
    return "World " + std::to_string(slot + 1);
}

/**
 * blank = random. all digits = that number. anything else (like a word) is turned into a number,
 * so any text works as a seed and the same text always gives the same world.
 * unsigned numbers wrap round when they get too big instead of breaking, so long seeds still work
 */
unsigned int seed_from_text(const string &text)
{
    if (text.empty())
    {
        return rnd(1000000);
    }

    bool is_number = true;
    for (int i = 0; i < (int)text.length(); i++)
    {
        if (text[i] < '0' || text[i] > '9')
        {
            is_number = false;
        }
    }

    unsigned int seed = 0;
    for (int i = 0; i < (int)text.length(); i++)
    {
        if (is_number)
        {
            // moves the digits so far one place left and adds the next one, "123" -> 1, 12, 123
            seed = seed * 10 + (text[i] - '0');
        }
        else
        {
            // mixes every character in, so changing any letter changes the seed
            seed = seed * 31 + text[i];
        }
    }

    return seed;
}

WorldSettings settings_from_form(const CreateWorldForm &form)
{
    WorldSettings settings;

    settings.name = form.name;
    if (settings.name.empty())
    {
        settings.name = default_world_name(form.slot);
    }

    settings.seed = seed_from_text(form.seed_text);
    settings.is_superflat = form.is_superflat;
    return settings;
}

bool is_mouse_over(const Button &button)
{
    return point_in_rectangle(mouse_position(), button.area);
}

// mouse_clicked so one click is one press, not every frame it's held
bool button_clicked(const Button &button)
{
    return mouse_clicked(LEFT_BUTTON) && is_mouse_over(button);
}

// same as button_clicked, for going backwards through settings
bool button_right_clicked(const Button &button)
{
    return mouse_clicked(RIGHT_BUTTON) && is_mouse_over(button);
}

MenuChoice main_menu_choice(const MainMenu &menu)
{
    if (button_clicked(menu.play))
    {
        return MENU_PLAY;
    }
    if (button_clicked(menu.settings))
    {
        return MENU_SETTINGS;
    }
    if (button_clicked(menu.exit))
    {
        return MENU_EXIT;
    }
    return MENU_NONE;
}

PauseChoice pause_menu_choice(const PauseMenu &menu)
{
    if (button_clicked(menu.resume))
    {
        return PAUSE_RESUME;
    }
    if (button_clicked(menu.settings))
    {
        return PAUSE_SETTINGS;
    }
    if (button_clicked(menu.quit_to_title))
    {
        return PAUSE_QUIT_TO_TITLE;
    }
    return PAUSE_NONE;
}

WorldSelectChoice world_select_choice(const WorldSelectMenu &menu)
{
    WorldSelectChoice choice;
    choice.action = SELECT_NONE;
    choice.slot = -1;

    for (int i = 0; i < NUM_SAVE_SLOTS; i++)
    {
        if (button_clicked(menu.slots[i]))
        {
            choice.action = SELECT_SLOT;
            choice.slot = i;
        }

        // empty slots have no delete button, so a click there doesn't count
        if (menu.can_delete[i] && button_clicked(menu.deletes[i]))
        {
            choice.action = SELECT_DELETE;
            choice.slot = i;
        }
    }

    if (button_clicked(menu.back))
    {
        choice.action = SELECT_BACK;
    }

    return choice;
}

DeleteChoice delete_menu_choice(const DeleteMenu &menu)
{
    if (button_clicked(menu.confirm))
    {
        return DELETE_CONFIRM;
    }
    if (button_clicked(menu.cancel))
    {
        return DELETE_CANCEL;
    }
    return DELETE_NONE;
}

/**
 * the settings change straight away, so they're already in use when done is clicked.
 * main saves them to the file then
 */
bool handle_settings_menu(const SettingsMenu &menu, Settings &settings)
{
    for (int i = 0; i < NUM_SETTINGS; i++)
    {
        if (button_clicked(menu.options[i]))
        {
            next_option(settings, i);
        }
        else if (button_right_clicked(menu.options[i]))
        {
            previous_option(settings, i);
        }
    }

    return button_clicked(menu.done);
}

/**
 * splashkit collects typed keys into its own text while reading.
 * it starts from the text already in the box, so typing carries on from it
 */
void start_typing(CreateWorldForm &form, TextField field, const Button &box, const string &text)
{
    start_reading_text(box.area, text);
    form.active_field = field;
}

void stop_typing(CreateWorldForm &form)
{
    if (reading_text())
    {
        end_reading_text();
    }
    form.active_field = FIELD_NONE;
}

/**
 * copies what's been typed so far into the form, every frame a box is active.
 * enter or escape make splashkit stop reading by itself, so the box stops being active.
 * only copied while still reading because escape empties splashkit's text
 */
void update_typed_text(CreateWorldForm &form, const CreateWorldMenu &menu)
{
    if (form.active_field == FIELD_NONE)
    {
        return;
    }

    if (!reading_text())
    {
        form.active_field = FIELD_NONE;
        return;
    }

    Button box = menu.name_box;
    if (form.active_field == FIELD_SEED)
    {
        box = menu.seed_box;
    }

    string text = text_input();

    // too long: cut it down and restart reading from the shorter text,
    // otherwise splashkit keeps the extra letters and backspace deletes hidden ones first
    if ((int)text.length() > MAX_TEXT_LENGTH)
    {
        text = text.substr(0, MAX_TEXT_LENGTH);
        start_reading_text(box.area, text);
    }

    if (form.active_field == FIELD_NAME)
    {
        form.name = text;
    }
    else
    {
        form.seed_text = text;
    }
}

/**
 * typed text is copied first, so clicking another box starts it from the latest text.
 * clicking anywhere that isn't a box stops typing, including create and cancel
 */
CreateChoice handle_create_world_menu(const CreateWorldMenu &menu, CreateWorldForm &form)
{
    update_typed_text(form, menu);

    if (button_clicked(menu.name_box))
    {
        start_typing(form, FIELD_NAME, menu.name_box, form.name);
    }
    else if (button_clicked(menu.seed_box))
    {
        start_typing(form, FIELD_SEED, menu.seed_box, form.seed_text);
    }
    else if (mouse_clicked(LEFT_BUTTON))
    {
        stop_typing(form);
    }

    if (button_clicked(menu.type))
    {
        form.is_superflat = !form.is_superflat;
    }
    if (button_clicked(menu.create))
    {
        return CREATE_CREATE;
    }
    if (button_clicked(menu.cancel))
    {
        return CREATE_CANCEL;
    }
    return CREATE_NONE;
}

/**
 * draw_text puts the top left of the text at x, y,
 * so half the text's size is taken off to put its middle there instead
 */
void draw_centred_text(const string &text, color colour, int size, double centre_x, double centre_y)
{
    double width = text_width(text, MENU_FONT, size);
    double height = text_height(text, MENU_FONT, size);
    draw_text(text, colour, MENU_FONT, size, centre_x - width / 2.0, centre_y - height / 2.0);
}

// box with a black border, hover_fill while the mouse is over it so you can tell it's clickable
void draw_button_in_colour(const Button &button, color fill, color hover_fill)
{
    if (is_mouse_over(button))
    {
        fill = hover_fill;
    }

    fill_rectangle(fill, button.area);
    draw_rectangle(COLOR_BLACK, button.area);

    double centre_x = button.area.x + button.area.width / 2.0;
    double centre_y = button.area.y + button.area.height / 2.0;
    draw_centred_text(button.label, COLOR_WHITE, BUTTON_TEXT_SIZE, centre_x, centre_y);
}

// the normal grey button
void draw_button(const Button &button)
{
    draw_button_in_colour(button, rgb_color(90, 90, 90), rgb_color(130, 130, 130));
}

/**
 * dark box with the text on the left. white border and _ on the end while it's being typed in.
 * an empty box that isn't active shows grey placeholder text: what you get if it's left blank
 */
void draw_text_box(const Button &box, const string &placeholder, bool is_active)
{
    fill_rectangle(rgb_color(40, 40, 40), box.area);

    string text = box.label;
    color text_colour = COLOR_WHITE;
    color border = COLOR_BLACK;

    if (is_active)
    {
        text += "_";
        border = COLOR_WHITE;
    }
    else if (text.empty())
    {
        text = placeholder;
        text_colour = COLOR_GRAY;
    }

    draw_rectangle(border, box.area);

    // left aligned, but still centred top to bottom
    double height = text_height(text, MENU_FONT, BUTTON_TEXT_SIZE);
    draw_text(text, text_colour, MENU_FONT, BUTTON_TEXT_SIZE, box.area.x + TEXT_BOX_PADDING, box.area.y + (box.area.height - height) / 2.0);
}

void draw_main_menu(const MainMenu &menu)
{
    clear_screen(COLOR_SKY_BLUE);

    // title a quarter of the way down, above the buttons
    draw_centred_text("Voxel Engine", COLOR_WHITE, TITLE_SIZE, screen_width() / 2.0, screen_height() / 4.0);

    draw_button(menu.play);
    draw_button(menu.settings);
    draw_button(menu.exit);
}

// every key and what it does, listed on the pause menu.
// first half goes in the left column (moving and the mouse), second half in the right (toggles)
const int NUM_CONTROLS = 12;
const string CONTROLS[NUM_CONTROLS] = {
    "WASD - move",
    "Space - jump/fly up",
    "Shift - fly down",
    "Mouse - look",
    "Left click - break",
    "Right click - place",
    "1-7 - pick block",
    "F - fly",
    "C - debug camera",
    "X - see-through",
    "F11 - fullscreen",
    "Esc - pause/resume"};

// smaller than the labels so all 6 rows fit under the buttons
const int CONTROLS_SIZE = 12;
const double CONTROLS_LINE_HEIGHT = 20.0;

// left edge of one column to the left edge of the next, the longest line is about 230 wide
const double CONTROLS_COLUMN_WIDTH = 250.0;

/**
 * a "Controls" heading, then two columns of 6 under it.
 * draw_text instead of draw_centred_text so each column lines up on its left edge
 */
void draw_controls(double top_y)
{
    double centre_x = screen_width() / 2.0;

    draw_centred_text("Controls", COLOR_WHITE, LABEL_SIZE, centre_x, top_y);

    int rows = NUM_CONTROLS / 2;

    // the two columns together are 2 column widths wide, so starting one width left of centre centres them
    double left_x = centre_x - CONTROLS_COLUMN_WIDTH;
    double first_line_y = top_y + CONTROLS_LINE_HEIGHT;

    for (int i = 0; i < NUM_CONTROLS; i++)
    {
        int column = i / rows; // 0 for the first 6, 1 for the rest
        int row = i % rows;    // 0 to 5 down each column

        draw_text(CONTROLS[i], COLOR_WHITE, MENU_FONT, CONTROLS_SIZE, left_x + column * CONTROLS_COLUMN_WIDTH, first_line_y + row * CONTROLS_LINE_HEIGHT);
    }
}

/**
 * drawn over the world instead of clearing the screen
 * pause menu darkens bg gameplay
 */
void draw_pause_menu(const PauseMenu &menu)
{
    fill_rectangle(rgba_color(0,0,0,120), 0, 0, screen_width(), screen_height());

    draw_centred_text("Game Paused", COLOR_WHITE, TITLE_SIZE, screen_width() / 2.0, screen_height() / 4.0);

    draw_button(menu.resume);
    draw_button(menu.settings);
    draw_button(menu.quit_to_title);

    // one gap under the last button
    draw_controls(menu.quit_to_title.area.y + BUTTON_HEIGHT + BUTTON_GAP);
}

// how far the debug camera label sits in from the window's edges
const double HUD_MARGIN = 10.0;
const double DEBUG_DOT_RADIUS = 6.0;

// space inside the label's box, around the dot and text and between them
const double DEBUG_LABEL_PADDING = 8.0;

/**
 * dark see-through box in the bottom right corner with a red dot and red text,
 * the box keeps the red readable over bright sky and blocks.
 * the box is sized from the text, then everything is placed from its top left corner
 */
void draw_debug_cam_label()
{
    string text = "DEBUG CAMERA";
    double width = text_width(text, MENU_FONT, LABEL_SIZE);
    double height = text_height(text, MENU_FONT, LABEL_SIZE);

    // padding, dot, padding, text, padding
    double dot_space = DEBUG_DOT_RADIUS * 2.0 + DEBUG_LABEL_PADDING;
    double box_width = DEBUG_LABEL_PADDING + dot_space + width + DEBUG_LABEL_PADDING;
    double box_height = height + DEBUG_LABEL_PADDING * 2.0;

    // measured back from the bottom right corner
    double box_x = screen_width() - HUD_MARGIN - box_width;
    double box_y = screen_height() - HUD_MARGIN - box_height;

    fill_rectangle(rgba_color(0, 0, 0, 150), box_x, box_y, box_width, box_height);

    // the dot and the text both line up on the box's middle height
    double centre_y = box_y + box_height / 2.0;
    fill_circle(COLOR_RED, box_x + DEBUG_LABEL_PADDING + DEBUG_DOT_RADIUS, centre_y, DEBUG_DOT_RADIUS);
    draw_text(text, COLOR_RED, MENU_FONT, LABEL_SIZE, box_x + DEBUG_LABEL_PADDING + dot_space, centre_y - height / 2.0);
}

void draw_world_select_menu(const WorldSelectMenu &menu)
{
    clear_screen(COLOR_SKY_BLUE);

    draw_centred_text("Select World", COLOR_WHITE, TITLE_SIZE, screen_width() / 2.0, screen_height() / 4.0);

    for (int i = 0; i < NUM_SAVE_SLOTS; i++)
    {
        draw_button(menu.slots[i]);

        // red so it stands out from the slot as something that can't be undone
        if (menu.can_delete[i])
        {
            draw_button_in_colour(menu.deletes[i], rgb_color(160, 40, 40), rgb_color(210, 60, 60));
        }
    }
    draw_button(menu.back);
}

/**
 * says which world is about to go, so the wrong one isn't deleted by mistake.
 * between the title and the buttons, the name big and the warning smaller under it
 */
void draw_delete_menu(const DeleteMenu &menu, const string &world_name)
{
    clear_screen(COLOR_SKY_BLUE);

    double centre_x = screen_width() / 2.0;

    draw_centred_text("Delete World?", COLOR_WHITE, TITLE_SIZE, centre_x, screen_height() / 4.0);

    draw_centred_text(world_name, COLOR_WHITE, BUTTON_TEXT_SIZE, centre_x, menu.confirm.area.y - 75.0);
    draw_centred_text("will be gone forever", COLOR_WHITE, LABEL_SIZE, centre_x, menu.confirm.area.y - 40.0);

    // red like the X, so it's clear this is the one that deletes
    draw_button_in_colour(menu.confirm, rgb_color(160, 40, 40), rgb_color(210, 60, 60));
    draw_button(menu.cancel);
}

void draw_settings_menu(const SettingsMenu &menu)
{
    clear_screen(COLOR_SKY_BLUE);

    double centre_x = screen_width() / 2.0;

    // above the first row
    draw_centred_text("Settings", COLOR_WHITE, TITLE_SIZE, centre_x, menu.options[0].area.y - 50.0);

    for (int i = 0; i < NUM_SETTINGS; i++)
    {
        string name = setting_name(i);
        double width = text_width(name, MENU_FONT, BUTTON_TEXT_SIZE);
        double height = text_height(name, MENU_FONT, BUTTON_TEXT_SIZE);

        // right aligned so every name ends the same gap before its button
        double name_x = centre_x - BUTTON_GAP / 2.0 - width;
        double row_centre_y = menu.options[i].area.y + BUTTON_HEIGHT / 2.0;
        draw_text(name, COLOR_WHITE, MENU_FONT, BUTTON_TEXT_SIZE, name_x, row_centre_y - height / 2.0);

        draw_button(menu.options[i]);
    }

    draw_button(menu.done);

    // under done, so it's clear the buttons can go both ways
    draw_centred_text("Click to change, right click to go back", COLOR_WHITE, LABEL_SIZE, centre_x, menu.done.area.y + BUTTON_HEIGHT + 25.0);
}

void draw_create_world_menu(const CreateWorldMenu &menu, const CreateWorldForm &form)
{
    clear_screen(COLOR_SKY_BLUE);

    double centre_x = screen_width() / 2.0;

    // above the first label
    draw_centred_text("Create World", COLOR_WHITE, TITLE_SIZE, centre_x, menu.name_box.area.y - LABEL_SPACE - 50.0);

    // each label sits in the space left above its box
    draw_centred_text("World Name", COLOR_WHITE, LABEL_SIZE, centre_x, menu.name_box.area.y - LABEL_SPACE / 2.0);
    draw_centred_text("Seed", COLOR_WHITE, LABEL_SIZE, centre_x, menu.seed_box.area.y - LABEL_SPACE / 2.0);
    draw_centred_text("World Type", COLOR_WHITE, LABEL_SIZE, centre_x, menu.type.area.y - LABEL_SPACE / 2.0);

    draw_text_box(menu.name_box, default_world_name(form.slot), form.active_field == FIELD_NAME);
    draw_text_box(menu.seed_box, "Random", form.active_field == FIELD_SEED);

    draw_button(menu.type);
    draw_button(menu.create);
    draw_button(menu.cancel);
}

/**
 * refreshes the screen itself because building the world straight after
 * stops the loop, so main never gets to refresh until it's done
 */
void draw_loading_screen(const string &message)
{
    clear_screen(COLOR_SKY_BLUE);
    draw_centred_text(message, COLOR_WHITE, BUTTON_TEXT_SIZE, screen_width() / 2.0, screen_height() / 2.0);
    refresh_screen();
}

/**
 * opens the slot's save if it has one. if not, builds a new world from the settings
 * and saves it straight away, so the slot has files from the start.
 * a damaged save is rebuilt from its settings too, so the world still opens.
 * everything else is reset so nothing carries over from the last world played
 */
void start_world(int slot, const WorldSettings &settings, World &world, Camera &cam, Player &player, bool &is_camera_detached, bool &is_cursor_locked)
{
    // loading takes a moment, so show something instead of a frozen menu
    draw_loading_screen("Loading world...");

    if (!load_world(slot, world, cam, player))
    {
        draw_loading_screen("Generating world...");

        build_world(world, settings);

        cam.position = find_spawn(world);
        cam.yaw = 0.0;
        cam.pitch = 0.0;
        player.is_flying = false;

        save_world(slot, settings, world, cam, player);
    }

    // not saved, so these start the same whether the world was loaded or built
    player.velocity_y = 0.0;
    player.on_ground = false;
    is_camera_detached = false;

    // throws away any mouse movements from before to stop mouse movement on the first frame
    lock_cursor(is_cursor_locked);
}