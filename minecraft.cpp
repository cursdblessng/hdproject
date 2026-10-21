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

enum BlockType
{
    GRASS,
    DIRT,
    STONE,
    WOOD,
    LEAVES,
    WATER,
};

struct Block
{
    Vector3 position;
    BlockType type;
    bool is_active;
};

struct Camera
{
    Vector3 position;
    double yaw;   // rotation for left right
    double pitch; // rotation for up down
    double fov;
};

const int MAX_BLOCKS = 10000;

struct BlockArray
{
    Block data[MAX_BLOCKS];
    int size;
};

/**
 * the corners of a cube, in a struct so it can be
 * returned from functions without using pointers
 *
 * index layout:
 * bottom face (y + 0):  0 = (0,0,0)  1 = (1,0,0)  2 = (1,0,1)  3 = (0,0,1)
 * top face    (y + 1):  4 = (0,1,0)  5 = (1,1,0)  6 = (1,1,1)  7 = (0,1,1)
 *
 * corner i is directly below i + 4
 */
struct CubeCorners
{
    Vector3 points[8];
};

void init_block_array(BlockArray &arr)
{
    arr.size = 0;
}

void add_block(BlockArray &arr, Block b)
{
    if (arr.size < MAX_BLOCKS)
    {
        arr.data[arr.size] = b;
        arr.size++;
    }
    else
    {
        write_line("Maximum block count reached.");
    }
}

Vector2 project_point(const Camera &cam, const Vector3 &p)
{
    double dx = p.x - cam.position.x;
    double dy = p.y - cam.position.y;
    double dz = p.z - cam.position.z;

    double rx = dx * cos(-cam.yaw) - dz * sin(-cam.yaw);
    double rz = dx * sin(-cam.yaw) + dz * cos(-cam.yaw);

    double ry = dy * cos(-cam.pitch) - rz * sin(-cam.pitch);
    rz = dy * sin(-cam.pitch) + rz * cos(-cam.pitch);

    if (rz <= 0.1)
    {
        return {-1, -1};
    }

    double fov_scale = (screen_width() / 2.0) / tan(cam.fov / 2.0);

    double screen_x = (rx * fov_scale / rz) + (screen_width() / 2.0);
    // fixed ry idk wtf the error was previously
    double screen_y = (-ry * fov_scale / rz) + (screen_height() / 2.0);

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

// picks display color for blocks
color block_colour(BlockType type)
{
    switch (type)
    {
    case GRASS:
        return rgb_color(95, 159, 53);
    case DIRT:
        return rgb_color(134, 96, 67);
    case STONE:
        return rgb_color(125, 125, 125);
    case WOOD:
        return rgb_color(102, 81, 51);
    case LEAVES:
        return rgb_color(40, 110, 30);
    case WATER:
        return rgb_color(50, 90, 200);
    }
    return COLOR_WHITE; // fallback (should never happen)
}

// works out the 8 corner positions of a block
CubeCorners get_block_corners(const Block &b)
{
    CubeCorners c;

    double x = b.position.x;
    double y = b.position.y;
    double z = b.position.z;

    // bottom face of cube
    c.points[0] = {x,       y, z};
    c.points[1] = {x + 1.0, y, z};
    c.points[2] = {x + 1.0, y, z + 1.0};
    c.points[3] = {x,       y, z + 1.0};

    // top face, y values +1 because its above
    c.points[4] = {x,       y + 1.0, z};
    c.points[5] = {x + 1.0, y + 1.0, z};
    c.points[6] = {x + 1.0, y + 1.0, z + 1.0};
    c.points[7] = {x,       y + 1.0, z + 1.0};

    return c;
}

// draws the 12 edges of one block
void draw_wireframe_block(const Camera &cam, const Block &b)
{
    CubeCorners c = get_block_corners(b);
    color edge_colour = block_colour(b.type);

    // each pass of the loop draws 3 edges, 4 passes so 12 edges
    for (int i = 0; i < 4; i++)
    {
        int next = (i + 1) % 4; // wraps 3 back round to 0 to close the square

        draw_3d_line(cam, c.points[i],     c.points[next],     edge_colour); // bottom edge
        draw_3d_line(cam, c.points[i + 4], c.points[next + 4], edge_colour); // top edge
        draw_3d_line(cam, c.points[i],     c.points[i + 4],    edge_colour); // vertical edge
    }
}

void draw_world_wireframe(const Camera &cam, const BlockArray &world)
{
    for (int i = 0; i < world.size; i++)
    {
        if (world.data[i].is_active)
        {
            draw_wireframe_block(cam, world.data[i]);
        }
    }
}

void build_test_world(BlockArray &world)
{
    // 5 x 5 patch of grass
    for (int x = -2; x <= 2; x++)
    {
        for (int z = -2; z <= 2; z++)
        {
            add_block(world, {{(double)x, 0.0, (double)z}, GRASS, true});
        }
    }

    // 3 high wood block with 1 leaf block on top
    add_block(world, {{4.0, 0.0, 4.0}, WOOD, true});
    add_block(world, {{4.0, 1.0, 4.0}, WOOD, true});
    add_block(world, {{4.0, 2.0, 4.0}, WOOD, true});
    add_block(world, {{4.0, 3.0, 4.0}, LEAVES, true});

    // stone block floating on the sise
    add_block(world, {{-4.0, 5.0, 3.0}, STONE, true});
}

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
int main()
{
    open_window("poopcraft", 800, 600);

    Camera cam;
    // Moved camera up slightly so we can see the grid below us
    cam.position = {0, 2, -5};
    cam.yaw = 0.0;
    cam.pitch = 0.0;
    cam.fov = 1.0;

    BlockArray world;
    init_block_array(world);
    build_test_world(world);

    bool is_cursor_locked = true;

    hide_mouse();
    move_mouse(screen_width() / 2.0, screen_height() / 2.0);

    double mouse_sensitivity = 0.003;

    while (!quit_requested())
    {
        process_events();

        //inputs
        handle_cursor_toggle(is_cursor_locked);
        handle_movement(cam);

        if (is_cursor_locked)
        {
            handle_mouse_look(cam, mouse_sensitivity);
        }

        //drawing
        clear_screen(COLOR_SKY_BLUE);

        draw_grid(cam, 20);
        draw_world_wireframe(cam, world);

        //crosshair
        fill_circle(COLOR_WHITE, screen_width() / 2.0, screen_height() / 2.0, 2);

        refresh_screen(60);
    }

    close_window("poopcraft");
    return 0;
}