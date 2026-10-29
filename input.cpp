#include "input.h"
#include "splashkit.h"
#include <cmath> // sin, cos

// wasd movement relative to where camera is facing
void handle_movement(Camera &cam)
{
    double move_speed = 0.1;

    // direction the camera faces on the flat ground
    // ignores pitch so looking down doesn't slow you down

    double forward_x = -sin(cam.yaw);
    double forward_z = cos(cam.yaw);

    // direction the camera's right
    double right_x = cos(cam.yaw);
    double right_z = sin(cam.yaw);

    if (key_down(W_KEY))
    {
        cam.position.x += forward_x * move_speed;
        cam.position.z += forward_z * move_speed;
    }
    if (key_down(S_KEY))
    {
        cam.position.x -= forward_x * move_speed;
        cam.position.z -= forward_z * move_speed;
    }
    if (key_down(D_KEY))
    {
        cam.position.x += right_x * move_speed;
        cam.position.z += right_z * move_speed;
    }
    if (key_down(A_KEY))
    {
        cam.position.x -= right_x * move_speed;
        cam.position.z -= right_z * move_speed;
    }

    // up or down movement
    if (key_down(SPACE_KEY))
        cam.position.y += move_speed;
    if (key_down(LEFT_SHIFT_KEY))
        cam.position.y -= move_speed;
}

// mouse look, only called when the cursor is locked
void handle_mouse_look(Camera &cam, double mouse_sensitivity)
{
    const double PI = 3.14159265358979323846;

    // call mouse_movement once per frame and store the result
    // it returns the movement since it was last called
    // calling it a second time in the same frame returns (0,0)
    // which was previously the issue

    vector_2d movement = mouse_movement();

    cam.yaw -= movement.x * mouse_sensitivity;
    cam.pitch += movement.y * mouse_sensitivity;

    // clamping pitch so the camera doesnt flip upside down
    double pitch_limit = PI / 2.0;
    if (cam.pitch > pitch_limit)
    {
        cam.pitch = pitch_limit;
    }
    if (cam.pitch < -pitch_limit)
    {
        cam.pitch = -pitch_limit;
    }

    // keep snapping the mouse to the middle so it never hits the window edge
    move_mouse(screen_width() / 2.0, screen_height() / 2.0);
}

// escape key toggles whether or not cursor is locked
void handle_cursor_toggle(bool &is_cursor_locked)
{
    if (key_typed(ESCAPE_KEY))
    {
        is_cursor_locked = !is_cursor_locked;

        if (is_cursor_locked)
        {
            hide_mouse();
            move_mouse(screen_width() / 2.0, screen_height() / 2.0);
            mouse_movement(); // removes the jump caused by re entering game
        }
        else
        {
            show_mouse();
        }
    }
}
