#include "input.h"
#include "splashkit.h"
#include <cmath> // sin, cos, sqrt, ceil

// player size, 0.6 x 1.8 x 0.6
const double PLAYER_HEIGHT = 1.8; // 1.8 blocks tall
const double PLAYER_WIDTH = 0.6;  // 0.6 blocks wide

// camera sits 1.62 above the bottom of the player, 0.18 below the top of the head
const double EYE_HEIGHT = 1.62;

// speeds are in blocks per second, so movement is the same at any frame rate
const double WALK_SPEED = 4.3;
const double FLY_SPEED = 10.0;
const double DEBUG_FLY_SPEED = 20.0; // debug camera, faster because it's for looking around

// how much downward speed is added every second (blocks per second, per second)
const double GRAVITY = 32.0;

// how high a jump goes. a bit over 1 so your feet get above a block's top, not just level with it
const double JUMP_HEIGHT = 1.25;

// upward speed needed to reach JUMP_HEIGHT: gravity takes away GRAVITY speed per second,
// so from v = sqrt(2 * gravity * height) it slows to 0 exactly at that height.
// worked out from the other two, so changing gravity keeps the same jump height
const double JUMP_SPEED = sqrt(2.0 * GRAVITY * JUMP_HEIGHT);

// the fastest you can fall, in blocks per second
const double MAX_FALL_SPEED = 50.0;

// the longest one physics step can be. a slow frame is split into steps this short,
// so nothing moves 1 block or more in a single step (50 * 1/60 = 0.83 at full fall speed).
// moving 1 or more could skip right over a block, and the ceil() snap in
// move_player would put you on the wrong surface (or through the floor)
const double MAX_PHYSICS_STEP = 1.0 / 60.0;

// the space the player takes up, worked out from where their eyes (the camera) are
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
void move_player(Camera &cam, Player &player, const World &world, double dx, double dy, double dz)
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

/**
 * which way wasd is pushing on the flat ground, as a direction of length 1
 * (or 0 if nothing is held, or opposite keys cancel out)
 * the keys are added up first then scaled to length 1, otherwise two keys at
 * right angles (w + d) add up to length sqrt(2) = 1.41 and diagonal is 41% faster
 * ignores pitch so looking down doesn't slow you down
 * Vector2 only has x and y, so y holds the z direction
 */
Vector2 wasd_direction(const Camera &cam)
{
    // direction the camera faces on the flat ground
    double forward_x = -sin(cam.yaw);
    double forward_z = cos(cam.yaw);

    // direction the camera's right
    double right_x = cos(cam.yaw);
    double right_z = sin(cam.yaw);

    Vector2 direction = {0.0, 0.0};

    if (key_down(W_KEY))
    {
        direction.x += forward_x;
        direction.y += forward_z;
    }
    if (key_down(S_KEY))
    {
        direction.x -= forward_x;
        direction.y -= forward_z;
    }
    if (key_down(D_KEY))
    {
        direction.x += right_x;
        direction.y += right_z;
    }
    if (key_down(A_KEY))
    {
        direction.x -= right_x;
        direction.y -= right_z;
    }

    // dividing by the length scales it to 1 without changing which way it points
    // skipped at 0 (nothing held) because dividing by 0 breaks it
    double length = sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0.0)
    {
        direction.x /= length;
        direction.y /= length;
    }

    return direction;
}

/**
 * one short physics step of step_time seconds
 * speeds are per second so multiplying by step_time gives how far to move this step
 * velocity changes by GRAVITY * step_time, so falling builds up the same at any fps
 * direction is worked out once per frame by handle_movement and passed in
 */
void movement_step(Camera &cam, Player &player, const World &world, const Vector2 &direction, double step_time)
{
    double speed = WALK_SPEED;
    if (player.is_flying)
    {
        speed = FLY_SPEED;
    }

    // how far to move in this step
    double dx = direction.x * speed * step_time;
    double dy = 0.0;
    double dz = direction.y * speed * step_time;

    if (player.is_flying)
    {
        // flying: space/shift move straight up/down at a steady speed,
        // no gravity, so you hover when neither is held
        if (key_down(SPACE_KEY))
        {
            dy += FLY_SPEED * step_time;
        }
        if (key_down(LEFT_SHIFT_KEY))
        {
            dy -= FLY_SPEED * step_time;
        }
    }
    else
    {
        // walking: gravity pulls a bit harder every step. this runs even
        // while standing, so move_player keeps bumping into the ground and
        // keeps on_ground up to date (walk off a ledge -> on_ground goes false)
        player.velocity_y -= GRAVITY * step_time;

        // cap the fall speed (see MAX_FALL_SPEED)
        if (player.velocity_y < -MAX_FALL_SPEED)
        {
            player.velocity_y = -MAX_FALL_SPEED;
        }

        // only jump when standing on something, so you can't jump mid-air
        if (key_down(SPACE_KEY) && player.on_ground)
        {
            player.velocity_y = JUMP_SPEED;
        }

        dy = player.velocity_y * step_time;
    }

    move_player(cam, player, world, dx, dy, dz);
}

/**
 * wasd movement relative to where camera is facing, for dt seconds
 * a long frame is split into equal steps no longer than MAX_PHYSICS_STEP,
 * so collision still works when the fps is low
 * e.g. dt = 0.05 at 20 fps: ceil(0.05 / 0.0167) = 3 steps of 0.0167 each
 */
void handle_movement(Camera &cam, Player &player, const World &world, double dt)
{
    // no time passed, nothing to do (also stops a divide by 0 below)
    if (dt <= 0.0)
    {
        return;
    }

    // keys are only read once per frame, every step uses the same direction
    Vector2 direction = wasd_direction(cam);

    int steps = (int)ceil(dt / MAX_PHYSICS_STEP);
    double step_time = dt / steps;

    for (int i = 0; i < steps; i++)
    {
        movement_step(cam, player, world, direction, step_time);
    }
}

/**
 * mouse look, only called when the cursor is locked
 * moving the mouse sideways turns (yaw), up and down looks up and down (pitch)
 */
void handle_mouse_look(Camera &cam, double mouse_sensitivity)
{
    const double PI = 3.14159265358979323846;

    // mouse_movement gives how far the mouse moved since it was last called,
    // so it's called once and stored. a second call this frame would give (0, 0)
    vector_2d movement = mouse_movement();

    cam.yaw -= movement.x * mouse_sensitivity;
    cam.pitch += movement.y * mouse_sensitivity;

    // clamping pitch to straight up and straight down so the camera doesnt flip upside down
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

/**
 * hides the mouse and keeps it in the middle for mouse look
 * used by escape, play and resume
 */
void lock_cursor(bool &is_cursor_locked)
{
    is_cursor_locked = true;
    hide_mouse();
    move_mouse(screen_width() / 2.0, screen_height() / 2.0);

    // throws away everything the mouse did while unlocked, otherwise the camera jumps on the next frame
    mouse_movement();
}

// escape locks and unlocks the cursor. unlocked is the same as paused
void handle_cursor_toggle(bool &is_cursor_locked)
{
    if (key_typed(ESCAPE_KEY))
    {
        if (is_cursor_locked)
        {
            is_cursor_locked = false;
            show_mouse();
        }
        else
        {
            lock_cursor(is_cursor_locked);
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

/**
 * left click breaks the block under the crosshair by setting its cell back to air.
 * mouse_clicked (not mouse_down) so one click breaks one block, instead
 * of one block every frame the button is held
 */
void handle_block_break(const Camera &cam, World &world)
{
    if (mouse_clicked(LEFT_BUTTON))
    {
        RayHit hit = raycast_block(cam, world);

        // false when the ray missed or only hit the floor, so there's nothing to break
        if (hit.hit_block)
        {
            set_block(world, hit.block_x, hit.block_y, hit.block_z, AIR);
        }
    }
}

/**
 * number keys 1 to 7 choose which block type right click will place,
 * in the same order as the BlockType enum.
 * key_down would also work here, key_typed matches the other toggles
 */
void handle_block_select(BlockType &selected_block)
{
    if (key_typed(NUM_1_KEY))
    {
        selected_block = GRASS;
    }
    if (key_typed(NUM_2_KEY))
    {
        selected_block = DIRT;
    }
    if (key_typed(NUM_3_KEY))
    {
        selected_block = STONE;
    }
    if (key_typed(NUM_4_KEY))
    {
        selected_block = WOOD;
    }
    if (key_typed(NUM_5_KEY))
    {
        selected_block = LEAVES;
    }
    if (key_typed(NUM_6_KEY))
    {
        selected_block = WATER;
    }
    if (key_typed(NUM_7_KEY))
    {
        selected_block = SAND;
    }
}

/**
 * right click places the selected block type against whatever the crosshair is on,
 * in the empty cell just in front of it.
 * refuses to place a block that overlaps the player, because they would be stuck inside it.
 */
void handle_block_place(const Camera &cam, World &world, BlockType selected_block)
{
    if (mouse_clicked(RIGHT_BUTTON))
    {
        RayHit hit = raycast_block(cam, world);

        // nothing within reach to place against
        if (!hit.can_place)
        {
            return;
        }

        // would be inside the player
        if (boxes_overlap(block_box(hit.place_x, hit.place_y, hit.place_z), player_box(cam.position)))
        {
            return;
        }

        // passed both checks, so place the block in the world
        set_block(world, hit.place_x, hit.place_y, hit.place_z, selected_block);
    }
}

/**
 * f11 switches between fullscreen and an 800x600 window.
 * toggling fullscreen on its own just stretches the 800x600 image over the
 * screen (blurry and squashed), so the window is resized to the monitor's
 * size first, which makes splashkit draw at full resolution.
 * everything else uses screen_width() / screen_height(), so the rendering
 * adapts to the new size automatically
 */
void handle_fullscreen_toggle(bool is_cursor_locked)
{
    if (key_typed(F11_KEY))
    {
        // 0 is the display number
        int monitor_width = display_width(display_details(0));
        int monitor_height = display_height(display_details(0));

        if (!current_window_is_fullscreen())
        {
            // resize first then fullscreen to avoid the stretching
            resize_current_window(monitor_width, monitor_height);
            current_window_toggle_fullscreen();
        }
        else
        {
            // leave fullscreen and go back to windowed
            // also moves window to the middle of the monitor
            current_window_toggle_fullscreen();
            resize_current_window(WINDOWED_WIDTH, WINDOWED_HEIGHT);
            move_current_window_to((monitor_width - WINDOWED_WIDTH) / 2, (monitor_height - WINDOWED_HEIGHT) / 2);
        }

        // recentre the mouse, otherwise the window changing size counts as the mouse moving
        // and the camera jumps. same as what lock_cursor does
        if (is_cursor_locked)
        {
            move_mouse(screen_width() / 2.0, screen_height() / 2.0);
            mouse_movement();
        }
    }
}

// x toggles see-through faces, a debug view for checking which faces are drawn
void handle_see_through_toggle(bool &see_through)
{
    if (key_typed(X_KEY))
    {
        see_through = !see_through;
    }
}

/**
 * c detaches the view from the player, giving a free flying debug camera.
 * on detach the debug camera is set to a copy of the player's camera,
 * so the view starts exactly where you were instead of jumping somewhere
 */
void handle_camera_detach_toggle(bool &is_detached, Camera &debug_cam, const Camera &player_cam)
{
    if (key_typed(C_KEY))
    {
        is_detached = !is_detached;

        if (is_detached)
        {
            // copies position, yaw, pitch and fov all at once
            debug_cam = player_cam;
        }
    }
}

/**
 * moves a camera with no gravity and no collision, for the debug camera.
 * like handle_movement but moves the camera directly instead of going through move_player,
 * so it can fly through blocks. no collision means no need to split into steps
 */
void handle_free_fly(Camera &cam, double dt)
{
    double distance = DEBUG_FLY_SPEED * dt;

    // same direction as handle_movement, so diagonal isn't faster here either
    Vector2 direction = wasd_direction(cam);

    cam.position.x += direction.x * distance;
    cam.position.z += direction.y * distance;

    if (key_down(SPACE_KEY))
    {
        cam.position.y += distance;
    }
    if (key_down(LEFT_SHIFT_KEY))
    {
        cam.position.y -= distance;
    }
}