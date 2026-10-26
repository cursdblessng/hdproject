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

const int NUM_FACES = 6;

/**
 * which 4 corners make up each face
 * corners go round the edge of the face in order, so splitting it into
 * triangles (0,1,2) and (0,2,3) covers the whole face with no gaps
 */
const int FACE_CORNERS[NUM_FACES][4] = {
    {0, 1, 2, 3}, // 0 bottom (y)
    {4, 5, 6, 7}, // 1 top    (y + 1)
    {0, 1, 5, 4}, // 2 front  (z)
    {3, 2, 6, 7}, // 3 back   (z + 1)
    {0, 3, 7, 4}, // 4 left   (x)
    {1, 2, 6, 5}, // 5 right  (x + 1)
};

// the direction each face points outwards, same order as FACE_CORNERS
// used for back-face culling
const Vector3 FACE_NORMALS[NUM_FACES] = {
    {0, -1, 0}, // bottom
    {0, 1, 0},  // top
    {0, 0, -1}, // front
    {0, 0, 1},  // back
    {-1, 0, 0}, // left
    {1, 0, 0},  // right
};

// brightness of each face so you can tell the sides of a cube apart
// top is brightest like sunlight from above, bottom is darkest
const double FACE_SHADE[NUM_FACES] = {0.5, 1.0, 0.8, 0.8, 0.65, 0.65};

/**
 * one visible face waiting to be drawn
 * stores which block and which face instead of the projected points
 * to keep it small (16 bytes) so the list fits on the stack
 */
struct FaceRef
{
    int block_index; // index into world.data
    int face;        // 0 to 5, index into FACE_CORNERS
    double depth;    // squared distance from camera, for sorting
};

// a cube shows at most 3 faces, but 10000 is plenty for now and keeps
// the list at 160KB. Windows only gives the stack about 1MB
const int MAX_VISIBLE_FACES = 10000;

struct FaceList
{
    FaceRef data[MAX_VISIBLE_FACES];
    int size;
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

    // clang-format off
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
    // clang-format on

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

        draw_3d_line(cam, c.points[i], c.points[next], edge_colour);         // bottom edge
        draw_3d_line(cam, c.points[i + 4], c.points[next + 4], edge_colour); // top edge
        draw_3d_line(cam, c.points[i], c.points[i + 4], edge_colour);        // vertical edge
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

// middle point of one face, used for culling and depth sorting
Vector3 face_centre(const CubeCorners &c, int face)
{
    Vector3 centre = {0.0, 0.0, 0.0};

    for (int i = 0; i < 4; i++)
    {
        Vector3 p = c.points[FACE_CORNERS[face][i]];
        centre.x += p.x;
        centre.y += p.y;
        centre.z += p.z;
    }

    // average of the 4 corners
    centre.x /= 4.0;
    centre.y /= 4.0;
    centre.z /= 4.0;

    return centre;
}

/**
 * BACK FACE CULLING:
 * a face is only visible if the camera is on the same side its pointing towards
 * the dot product of the normal and the face-to-camera direction
 * is positive only when they point the same way, meaning face is facing camera
 */
bool is_face_visible(const Camera &cam, const CubeCorners &c, int face)
{
    Vector3 centre = face_centre(c, face);
    Vector3 normal = FACE_NORMALS[face];

    double to_cam_x = cam.position.x - centre.x;
    double to_cam_y = cam.position.y - centre.y;
    double to_cam_z = cam.position.z - centre.z;

    double dot = to_cam_x * normal.x + to_cam_y * normal.y + to_cam_z * normal.z;

    return dot > 0;
}

/**
 * squared distance and if a > b then a squared > b squared, so the order is the same
 */
double distance_squared(const Vector3 &a, const Vector3 &b)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    double dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}
// darkens a colour by factor between 0 and 1
color shade_colour(color base, double factor)
{
    return rgb_color((int)(red_of(base) * factor),
                     (int)(green_of(base) * factor),
                     (int)(blue_of(base) * factor));
}

/**
 * fills the list with every face that the camera is facing
 */
void collect_visible_faces(const Camera &cam, const BlockArray &world, FaceList &faces)
{
    faces.size = 0;

    for (int i = 0; i < world.size; i++)
    {
        if (!world.data[i].is_active)
        {
            continue;
        }

        CubeCorners c = get_block_corners(world.data[i]);

        for (int f = 0; f < NUM_FACES; f++)
        {
            if (is_face_visible(cam, c, f))
            {
                // list full, so stop adding.
                if (faces.size >= MAX_VISIBLE_FACES)
                {
                    return;
                }

                faces.data[faces.size] = {i, f, distance_squared(cam.position, face_centre(c, f))};
                faces.size++;
            }
        }
    }
}

/**
 * painter's algorithm needs furthest faces first
 * insertion sort is used to take each face and slide it past
 * every face that is closer to the camera.
 * good enough for now
 *
 * to-do: CHANGE SORTING ALGORITHM LATER FOR BIGGER WORLDS
 */
void sort_faces_far_to_close(FaceList &faces)
{
    for (int i = 0; i < faces.size; i++)
    {
        FaceRef current = faces.data[i];
        int j = i - 1;

        while (j >= 0 && faces.data[j].depth < current.depth)
        {
            faces.data[j + 1] = faces.data[j];
            j--;
        }

        faces.data[j + 1] = current;
    }
}

// render block as two triangles on one side of block
void draw_face(const Camera &cam, const Block &b, int face)
{
    CubeCorners c = get_block_corners(b);
    Vector2 screen[4];

    for (int i = 0; i < 4; i++)
    {
        screen[i] = project_point(cam, c.points[FACE_CORNERS[face][i]]);

        // a corner is behind the camera, skip the entire face
        // near plane clipping, will fix this later
        if (screen[i].x == -1)
        {
            return;
        }
    }

    color fill = shade_colour(block_colour(b.type), FACE_SHADE[face]);

    // a quad is two triangles sharing a diagonal
    fill_triangle(fill, screen[0].x, screen[0].y, screen[1].x, screen[1].y, screen[2].x, screen[2].y);
    fill_triangle(fill, screen[0].x, screen[0].y, screen[2].x, screen[2].y, screen[3].x, screen[3].y);

    // darker outline so blocks next to each other don't merge into one blob
    color outline = shade_colour(fill,0.6);
    for (int i = 0; i < 4; i++)
    {
        int next = (i + 1) % 4;
        draw_line(outline, screen[i].x, screen[i].y, screen[next].x, screen[next].y);
    }
}

// collect then sort then draw far to near
void draw_world_solid(const Camera &cam, const BlockArray &world)
{
    // made fresh every frame because the camera moves, so which faces are are visible and their order changes each frame
    FaceList faces;

    collect_visible_faces(cam, world, faces);
    sort_faces_far_to_close(faces);

    for (int i = 0; i < faces.size; i++)
    {
        FaceRef f = faces.data[i];
        draw_face(cam, world.data[f.block_index], f.face);
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

        // inputs
        handle_cursor_toggle(is_cursor_locked);
        handle_movement(cam);

        if (is_cursor_locked)
        {
            handle_mouse_look(cam, mouse_sensitivity);
        }

        // drawing
        clear_screen(COLOR_SKY_BLUE);

        draw_grid(cam, 20);
        draw_world_solid(cam, world);

        // crosshair
        fill_circle(COLOR_WHITE, screen_width() / 2.0, screen_height() / 2.0, 2);

        refresh_screen(60);
    }

    close_window("poopcraft");
    return 0;
}