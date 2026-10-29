#include "input.h"
#include "splashkit.h"
#include <cmath> // sin, cos

// player size, 0.6x2.0x0.6
const double PLAYER_HEIGHT = 2.0; // 2 blocks tall
const double PLAYER_WIDTH = 0.6;  // 0.6 blocks wide

// camera sits 1.5 blocks above bottom of the player, middle of top block
const double EYE_HEIGHT = 1.5;

// the grid is at y = 0, treat it as solid ground
// so walking off grass patch doesnt fall
const double WORLD_FLOOR_Y = 0.0;

// how much downward speed is added every frame (blocks per frame, per frame)
const double GRAVITY = 0.008;

// upward speed given at the start of a jump. with GRAVITY = 0.008 this
// rises about 1.3 blocks, just enough to jump up onto a block
const double JUMP_SPEED = 0.14;

// the fastest you can fall. MUST stay below 1 block per frame, otherwise
// one step could skip right over a block and the ceil() snap in
// move_player would put you on the wrong surface (or through the floor)
const double MAX_FALL_SPEED = 0.5;

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
 * moves the camera by (dx, dy, dz), unless that would put the player
 * inside a block. each axis is tried on its own, so walking into a wall
 * at an angle only cancels the part going into the wall and you slide
 * along it instead of stopping dead
 * also works out whether the player is standing on something
 */
void move_player(Camera &cam, Player &player, const BlockArray &world, double dx, double dy, double dz)
{
    // try x: work out where we'd be, only go there if it's not inside a block
    Vector3 next = cam.position;
    next.x += dx;
    if (!box_hits_world(player_box(next), world))
    {
        cam.position = next;
    }

    // try z, starting from wherever the x step left us
    next = cam.position;
    next.z += dz;
    if (!box_hits_world(player_box(next), world))
    {
        cam.position = next;
    }

    // try y (falling, jumping, flying up and down)
    next = cam.position;
    next.y += dy;

    // blocked by floor, or bottom of player would go through the floor
    double next_feet = next.y - EYE_HEIGHT;
    bool blocked = box_hits_world(player_box(next), world) || next_feet < WORLD_FLOOR_Y;

    if (!blocked)
    {
        // moved freely, so nothing is under player
        cam.position = next;
        player.on_ground = false;
    }
    else if (dy < 0)
    {
        // falling and hit something: land with feet exactly on top of it.
        // every block top (and the floor) is a whole number, so the surface
        // is the next whole number above where the feet would have ended up
        cam.position.y = ceil(next_feet) + EYE_HEIGHT;
        player.velocity_y = 0.0;
        player.on_ground = true;
    }
    else if (dy > 0)
    {
        // going up and hit a ceiling: put the top of the head exactly at
        // the underside of the block, which is the whole number below
        double next_head = next_feet + PLAYER_HEIGHT;
        cam.position.y = floor(next_head) - PLAYER_HEIGHT + EYE_HEIGHT;
        player.velocity_y = 0.0;
    }
}

// wasd movement relative to where camera is facing
// works out how much the player WANTS to move. move_player then checks for collision
void handle_movement(Camera &cam, Player &player, const BlockArray &world)
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

    if (player.is_flying)
    {
        // flying: space/shift move straight up/down at a steady speed,
        // no gravity, so you hover when neither is held
        if (key_down(SPACE_KEY))
        {
            dy += move_speed;
        }
        if (key_down(LEFT_SHIFT_KEY))
        {
            dy -= move_speed;
        }
    }
    else
    {
        // walking: gravity pulls a bit harder every frame. this runs even
        // while standing, so move_player keeps bumping into the ground and
        // keeps on_ground up to date (walk off a ledge -> on_ground goes false)
        player.velocity_y -= GRAVITY;

        // cap the fall speed (see MAX_FALL_SPEED for why)
        if (player.velocity_y < -MAX_FALL_SPEED)
        {
            player.velocity_y = -MAX_FALL_SPEED;
        }

        // only jump when standing on something, so you can't jump mid-air
        if (key_down(SPACE_KEY) && player.on_ground)
        {
            player.velocity_y = JUMP_SPEED;
        }

        dy = player.velocity_y;
    }

    move_player(cam, player, world, dx, dy, dz);
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

// f toggles flying on/off
void handle_fly_toggle(Player &player)
{
    if (key_typed(F_KEY))
    {
        player.is_flying = !player.is_flying;

        // cancel any fall or jump that is in progress
        // so switching to flying mid-fall stops all momentum
        player.velocity_y = 0.0;
    }
}
