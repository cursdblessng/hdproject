#include "splashkit.h"
#include <cmath> // REQUIRED for cos, sin, tan

struct Vector3
{
    double x;
    double y;
    double z;
};

struct Vector2
{
    double x;
    double y;
};

struct Camera
{
    Vector3 position;
    double fov;
};

Vector2 project_point(const Camera &cam, const Vector3 &p)
{
    double dx = p.x - cam.position.x;
    double dy = p.y - cam.position.y;
    double dz = p.z - cam.position.z;

    if (dz <= 0.1)
    {
        return {-1, -1};
    }

    double fov_scale = (screen_width() / 2.0) / tan(cam.fov / 2.0);

    double screen_x = (dx * fov_scale / dz) + (screen_width() / 2.0);
    double screen_y = (-dy * fov_scale / dz) + (screen_height() / 2.0);

    return {screen_x, screen_y};
}

void draw_3d_line(const Camera &cam, const Vector3 &start, const Vector3 &end, color line_color)
{
    Vector2 screen_start = project_point(cam, start);
    Vector2 screen_end = project_point(cam, end);

    if (screen_start.x != -1 && screen_end.x != -1)
    {
        draw_line(line_color, screen_start.x, screen_start.y, screen_end.x, screen_end.y);
    }
}

void draw_grid(const Camera &cam, int grid_radius)
{
    color grid_color = rgba_color(100, 100, 100, 100);

    for (int i = -grid_radius; i <= grid_radius; i++)
    {
        // FIXED: Explicitly cast all ints to doubles in the initializer lists
        draw_3d_line(cam,
                     {(double)-grid_radius, 0.0, (double)i},
                     {(double)grid_radius, 0.0, (double)i},
                     grid_color);

        draw_3d_line(cam,
                     {(double)i, 0.0, (double)-grid_radius},
                     {(double)i, 0.0, (double)grid_radius},
                     grid_color);
    }
}

// moves the camera along the world axes
void handle_movement(Camera &cam)
{
    double move_speed = 0.1;

    if (key_down(W_KEY))
        cam.position.z += move_speed;
    if (key_down(S_KEY))
        cam.position.z -= move_speed;
    if (key_down(D_KEY))
        cam.position.x += move_speed;
    if (key_down(A_KEY))
        cam.position.x -= move_speed;

    // up or down movement
    if (key_down(SPACE_KEY))
        cam.position.y += move_speed;
    if (key_down(LEFT_SHIFT_KEY))
        cam.position.y -= move_speed;
}

int main()
{
    open_window("poopcraft", 800, 600);

    Camera cam;
    // Moved camera up slightly so we can see the grid below us
    cam.position = {0, 2, -5};
    cam.fov = 1.0;

    while (!quit_requested())
    {
        process_events();

        handle_movement(cam);

        clear_screen(COLOR_SKY_BLUE);

        draw_grid(cam, 20);

        refresh_screen(60);
    }

    close_window("poopcraft");
    return 0;
}
