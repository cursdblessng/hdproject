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