#include "menu.h"

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

// same layout as main menu so buttons sit in the same place
PauseMenu make_pause_menu()
{
    double centre_x = screen_width() / 2.0;
    double first_y = screen_height() / 2.0;
    double spacing = BUTTON_HEIGHT + BUTTON_GAP;

    PauseMenu menu;
    menu.resume = make_button("Resume", centre_x, first_y);
    menu.quit_to_title = make_button("Quit to Title", centre_x, first_y + spacing);
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
    }

    // under the last slot
    menu.back = make_button("Back", centre_x, first_y + NUM_SAVE_SLOTS * spacing);
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
    }

    if (button_clicked(menu.back))
    {
        choice.action = SELECT_BACK;
    }

    return choice;
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

// grey box with a black border, lighter while the mouse is over it so you can tell it's clickable
void draw_button(const Button &button)
{
    color fill = rgb_color(90, 90, 90);
    if (is_mouse_over(button))
    {
        fill = rgb_color(130, 130, 130);
    }

    fill_rectangle(fill, button.area);
    draw_rectangle(COLOR_BLACK, button.area);

    double centre_x = button.area.x + button.area.width / 2.0;
    double centre_y = button.area.y + button.area.height / 2.0;
    draw_centred_text(button.label, COLOR_WHITE, BUTTON_TEXT_SIZE, centre_x, centre_y);
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

/**
 * drawn over the world instead of clearing the screen
 * pause menu darkens bg gameplay
 */
void draw_pause_menu(const PauseMenu &menu)
{
    fill_rectangle(rgba_color(0,0,0,120), 0, 0, screen_width(), screen_height());

    draw_centred_text("Game Paused", COLOR_WHITE, TITLE_SIZE, screen_width() / 2.0, screen_height() / 4.0);

    draw_button(menu.resume);
    draw_button(menu.quit_to_title);
}

void draw_world_select_menu(const WorldSelectMenu &menu)
{
    clear_screen(COLOR_SKY_BLUE);

    draw_centred_text("Select World", COLOR_WHITE, TITLE_SIZE, screen_width() / 2.0, screen_height() / 4.0);

    for (int i = 0; i < NUM_SAVE_SLOTS; i++)
    {
        draw_button(menu.slots[i]);
    }
    draw_button(menu.back);
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
void draw_loading_screen()
{
    clear_screen(COLOR_SKY_BLUE);
    draw_centred_text("Generating world...", COLOR_WHITE, BUTTON_TEXT_SIZE, screen_width() / 2.0, screen_height() / 2.0);
    refresh_screen();
}

/**
 * the same for a new world or an existing one.
 * everything is reset so nothing carries over from the last world played
 */
void start_world(World &world, const WorldSettings &settings, Camera &cam, Player &player, bool &is_camera_detached, bool &is_cursor_locked)
{
    // building takes a moment, so show something instead of a frozen menu
    draw_loading_screen();

    build_world(world, settings);

    cam.position = find_spawn(world);
    cam.yaw = 0.0;
    cam.pitch = 0.0;
    player.velocity_y = 0.0;
    player.on_ground = false;
    player.is_flying = false;
    is_camera_detached = false;

    // throws away any mouse movements from before to stop mouse movement on the first frame
    lock_cursor(is_cursor_locked);
}