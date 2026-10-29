#include "input.h"
#include "splashkit.h"
#include <cmath> // sin, cos

// player size, 0.6x2.0x0.6
const double PLAYER_HEIGHT = 2.0; // 2 blocks tall
const double PLAYER_WIDTH = 0.6;  // 0.6 blocks wide

// camera sits 1.5 blocks above bottom of the player, middle of top block
const double EYE_HEIGHT = 1.5;

Box player_box(const Vector3 &eye)
{
    // the box spreads out evenly either side of the eyes on x and z
    double half_width = PLAYER_WIDTH / 2.0;

    // feet are EYE_HEIGHT below the eyes
    double feet_y = eye.y - EYE_HEIGHT;

    Box b;
    b.min = {eye.x - half_width, feet_y, eye.z - half_width};
    b.max = {eye.x + half_width, feet_y + PLAYER_HEIGHT, eye.z + half_width};
    return b;
}

/**
 * moves the camera by (dx, dy, dz)
 * UNLESS that would put the player inside a block.
 * each axis is tried on its own
 * so walking into a wall at an angle only cancels the part going into the wall
 * and you end up sliding alongside it instead of stopping completely
 */
void move_player(Camera &cam, const BlockArray &world, double dx, double dy, double dz)
{
    // try x: work out where we'd be, only go there if it's not inside a block
    Vector3 next = cam.position;
    next.x += dx;
    if (!box_hits_world(player_box(next), world))
    {
        cam.position = next;
    }

    next = cam.position;
    next.y += dy;
    if (!box_hits_world(player_box(next), world))
    {
        cam.position = next;
    }

    next = cam.position;
    next.z += dz;
    if (!box_hits_world(player_box(next), world))
    {
        cam.position = next;
    }
}

// wasd movement relative to where camera is facing
// works out how much the player WANTS to move. move_player then checks for collision
void handle_movement(Camera &cam, const BlockArray &world)
{
    double move_speed = 0.1;

    // direction the camera faces on the flat ground
    // ignores pitch so looking down doesn't slow you down

    double forward_x = -sin(cam.yaw);
    double forward_z = cos(cam.yaw);

    // direction the camera's right
    double right_x = cos(cam.yaw);
    double right_z = sin(cam.yaw);

    // total movement wanted this frame, added up from every key held
    double dx = 0.0;
    double dy = 0.0;
    double dz = 0.0;

    if (key_down(W_KEY))
    {
        dx += forward_x * move_speed;
        dz += forward_z * move_speed;
    }
    if (key_down(S_KEY))
    {
        dx -= forward_x * move_speed;
        dz -= forward_z * move_speed;
    }
    if (key_down(D_KEY))
    {
        dx += right_x * move_speed;
        dz += right_z * move_speed;
    }
    if (key_down(A_KEY))
    {
        dx -= right_x * move_speed;
        dz -= right_z * move_speed;
    }

    // up or down movement
    if (key_down(SPACE_KEY))
    {
        dy += move_speed;
    }
    if (key_down(LEFT_SHIFT_KEY))
    {
        dy -= move_speed;
    }

    move_player(cam, world, dx, dy, dz);
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
