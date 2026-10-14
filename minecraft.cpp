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
    cam.position = {0, 0, -5};
    cam.fov = 1.0;

    // single point in the world to test the projection with
    Vector3 test_point = {0, 0, 0};

    while (!quit_requested())
    {
        process_events();

        handle_movement(cam);

        clear_screen(COLOR_SKY_BLUE);

        Vector2 dot = project_point(cam, test_point);
        if (dot.x != -1)
        {
            fill_circle(COLOR_RED, dot.x, dot.y, 5);
        }

        refresh_screen(60);
    }

    close_window("poopcraft");
    return 0;
}
