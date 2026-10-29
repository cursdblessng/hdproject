#include "render.h"
#include "splashkit.h"
#include <cmath> // cos, sin, tan


// ============================================================
// CONSTANTS AND STRUCTS
// ============================================================

// how far in front of the camera the "near wall" is
// anything closer than this gets cut off, because dividing by a z
// thats tiny or negative blows up or flips the point
const double NEAR_PLANE = 0.1;

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

// a flat shape with a variable number of corners, used for clipping
// each straight cut can add at most 1 corner: 4 corners, +1 from the
// near wall, +1 from each of the 4 screen edges = 9 at most
// 12 leaves room to spare
const int MAX_POLYGON_POINTS = 12;

struct Polygon
{
    Vector3 points[MAX_POLYGON_POINTS];
    int size;
};

// same idea as Polygon, but for points already projected onto the
// screen (pixels), used for cutting a face to the window's edges
struct ScreenPolygon
{
    Vector2 points[MAX_POLYGON_POINTS];
    int size;
};

// the 4 edges of the window, used to cut faces into the visible area
enum ScreenEdge
{
    EDGE_LEFT,  // x = 0
    EDGE_RIGHT, // x = screen_width()
    EDGE_TOP,   // y = 0
    EDGE_BOTTOM // y = screen_height()
};


// ============================================================
// PROJECTION (world -> camera space -> screen)
// ============================================================

/**
 * step 1 of projection: moves and rotates a world point so the
 * camera is at (0,0,0) looking straight down +z
 * the result is in "camera space":
 * x = how far right of the camera
 * y = how far above the camera
 * z = how far in front of the camera (negative = behind)
 */
Vector3 world_to_camera(const Camera &cam, const Vector3 &p)
{
    // position relative to the camera
    double dx = p.x - cam.position.x;
    double dy = p.y - cam.position.y;
    double dz = p.z - cam.position.z;

    // undo the camera's yaw (turn left/right)
    double rx = dx * cos(-cam.yaw) - dz * sin(-cam.yaw);
    double rz = dx * sin(-cam.yaw) + dz * cos(-cam.yaw);

    // undo the camera's pitch (look up/down)
    double ry = dy * cos(-cam.pitch) - rz * sin(-cam.pitch);
    rz = dy * sin(-cam.pitch) + rz * cos(-cam.pitch);

    return {rx, ry, rz};
}

/**
 * step 2 of projection: perspective divide, camera space -> screen pixel
 * further away means bigger z, so closer to middle of screen
 * only calls this when c.z >= NEAR_PLANE, otherwise divide the breaks
 */
Vector2 camera_to_screen(const Camera &cam, const Vector3 &c)
{
    double fov_scale = (screen_width() / 2.0) / tan(cam.fov / 2.0);

    double screen_x = (c.x * fov_scale / c.z) + (screen_width() / 2.0);
    // minus because screen y down means camera y up
    double screen_y = (-c.y * fov_scale / c.z) + (screen_height() / 2.0);

    return {screen_x, screen_y};
}


// ============================================================
// CLIPPING (near wall and window edges)
// ============================================================

/**
 * finds where the line from a to b crosses the near wall
 * a and b must be in camera space, with one in front of the wall
 * and one behind the wall (so a.z and b.z are never equal, never give 0 when divided)
 */
Vector3 near_plane_intersection(const Vector3 &a, const Vector3 &b)
{
    // how far along the line the wall is: 0 = at a, 1 = at b, 0.5 = halfway
    double t = (NEAR_PLANE - a.z) / (b.z - a.z);

    // move that fraction of the way from a towards b on x and y
    // z is exactly on the wall by definition
    return {a.x + t * (b.x - a.x),
            a.y + t * (b.y - a.y),
            NEAR_PLANE};
}

/**
 * cuts a polygon (in camera space) along the near wall
 * and keeps the part in front of it
 * goes around every edge and:
 *      keep the start corner if its in front of the wall
 *      add the crossing point if the edge goes through the wall
 */
Polygon clip_polygon_to_near_plane(const Polygon &input)
{
    Polygon output;
    output.size = 0;

    for (int i = 0; i < input.size; i++)
    {
        // this edge runs from current to next, the last corner wraps
        // back round to the first to close the loop
        Vector3 current = input.points[i];
        Vector3 next = input.points[(i + 1) % input.size];

        bool current_in_front = current.z >= NEAR_PLANE;
        bool next_in_front = next.z >= NEAR_PLANE;

        // rule 1: corners in front of the wall stay
        if (current_in_front)
        {
            output.points[output.size] = current;
            output.size++;
        }

        // rule 2: one end in front and one behind,
        // so the edge crosses the wall, crossing point becomes new corner
        if (current_in_front != next_in_front)
        {
            output.points[output.size] = near_plane_intersection(current, next);
            output.size++;
        }
    }

    return output;
}

// is this pixel on the window side of the given edge?
bool is_inside_egde(const Vector2 &p, ScreenEdge edge)
{
    switch (edge)
    {
    case EDGE_LEFT:
        return p.x >= 0;
    case EDGE_RIGHT:
        return p.x <= screen_width();
    case EDGE_TOP:
        return p.y >= 0;
    case EDGE_BOTTOM:
        return p.y <= screen_height();
    }
    return true; // fallback, but should never occur
}

/**
 * finds where the line from a to b crosses the given window edge
 * same idea as near_plane_intersection, but in 2D:
 *      t = how far along the line the edge is at (0 at a, 1 at b)
 *      only called when one end is outside and one is inside
 *      so the division is never with zero.
 */
Vector2 edge_intersection(const Vector2 &a, const Vector2 &b, ScreenEdge edge)
{
    double t = 0;

    switch (edge)
    {
    case EDGE_LEFT:
        t = (0 - a.x) / (b.x - a.x);
        break;
    case EDGE_RIGHT:
        t = (screen_width() - a.x) / (b.x - a.x);
        break;
    case EDGE_TOP:
        t = (0 - a.y) / (b.y - a.y);
        break;
    case EDGE_BOTTOM:
        t = (screen_height() - a.y) / (b.y - a.y);
        break;
    }

    // move that fraction of the way from a to b
    return {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
}

/**
 * cuts a screen polygon along one window edge, keeping the inside part
 * exactly the same two rules as clip_polygon_to_near_plane:
 *      keep the start corner if it's inside
 *      add the crossing point if the edge goes through the window edge
 */
ScreenPolygon clip_to_screen_edge(const ScreenPolygon &input, ScreenEdge edge)
{
    ScreenPolygon output;
    output.size = 0;

    for (int i = 0; i < input.size; i++)
    {
        Vector2 current = input.points[i];
        Vector2 next = input.points[(i + 1) % input.size];

        bool current_inside = is_inside_egde(current, edge);
        bool next_inside = is_inside_egde(next, edge);

        // first rule: corners inside the window stay.
        if (current_inside)
        {
            output.points[output.size] = current;
            output.size++;
        }

        // second rule: edge crosses the window edge, so add the crossing point
        if (current_inside != next_inside)
        {
            output.points[output.size] = edge_intersection(current, next, edge);
            output.size++;
        }
    }

    return output;
}

/**
 * cuts a screen polygon down to the window, one edge at a time
 * each cut works on the result of the cut before it, so after all
 * 4 every corner is inside the window (small, safe pixel values
 * that SplashKit's triangle filling can handle)
 */
ScreenPolygon clip_polygon_to_screen(const ScreenPolygon &input)
{
    ScreenPolygon result = input;

    result = clip_to_screen_edge(result, EDGE_LEFT);
    result = clip_to_screen_edge(result, EDGE_RIGHT);
    result = clip_to_screen_edge(result, EDGE_TOP);
    result = clip_to_screen_edge(result, EDGE_BOTTOM);

    return result;
}


// ============================================================
// LINES AND WIREFRAME
// ============================================================

void draw_3d_line(const Camera &cam, const Vector3 &start, const Vector3 &end, color line_color)
{
    Vector3 a = world_to_camera(cam, start);
    Vector3 b = world_to_camera(cam, end);

    // >= not > because a clipped end sits exactly ON the wall and must count
    bool a_in_front = a.z >= NEAR_PLANE;
    bool b_in_front = b.z >= NEAR_PLANE;

    if (!a_in_front && !b_in_front)
    {
        return;
    }

    // one end behind: slide it forwards along the line onto the wall
    // (only one of these can run, both-behind already returned above)
    if (!a_in_front)
    {
        a = near_plane_intersection(a, b);
    }
    if (!b_in_front)
    {
        b = near_plane_intersection(a, b);
    }

    // both ends are now safely in front, so step 3 (divide) is safe
    Vector2 screen_start = camera_to_screen(cam, a);
    Vector2 screen_end = camera_to_screen(cam, b);

    draw_line(line_color, screen_start.x, screen_start.y, screen_end.x, screen_end.y);
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


// ============================================================
// SOLID FACES (culling, painter's algorithm)
// ============================================================

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
    for (int i = 1; i < faces.size; i++)
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

// projects one face to the screen, cutting off any part behind the near wall
void draw_face(const Camera &cam, const Block &b, int face)
{
    CubeCorners c = get_block_corners(b);

    // projection steps 1 and 2 for the face's 4 corners
    // stays in camera space so clipping can see each corner's depth
    Polygon face_shape;
    face_shape.size = 4;
    for (int i = 0; i < 4; i++)
    {
        face_shape.points[i] = world_to_camera(cam, c.points[FACE_CORNERS[face][i]]);
    }

    // cut off whatever is behind the near wall
    Polygon clipped = clip_polygon_to_near_plane(face_shape);

    // whole face was behind the wall, nothing left to fill
    if (clipped.size < 3)
    {
        return;
    }

    // step 3: every remaining corner is in front of the wall, so can divide
    ScreenPolygon screen_shape;
    screen_shape.size = clipped.size;
    for (int i = 0; i < clipped.size; i++)
    {
        screen_shape.points[i] = camera_to_screen(cam, clipped.points[i]);
    }

    // corners close to the camera can land tens of thousands of pixels
    // off screen, which overflows SplashKit's triangle filling (it uses
    // 16-bit numbers, max 32767) and causes the lag and random shapes.
    // cutting to the window keeps every corner inside it
    ScreenPolygon on_screen = clip_polygon_to_screen(screen_shape);

    // face is entirely off the size of the window, so dont render
    if (on_screen.size < 3)
    {
        return;
    }

    color fill = shade_colour(block_colour(b.type), FACE_SHADE[face]);

    // triangle fan: every triangle shares corner 0
    // starts at 1, starting with 0 would give 0,0,1, a triangle with no area
    for (int i = 1; i < on_screen.size - 1; i++)
    {
        fill_triangle(fill,
                      on_screen.points[0].x, on_screen.points[0].y,
                      on_screen.points[i].x, on_screen.points[i].y,
                      on_screen.points[i + 1].x, on_screen.points[i + 1].y);
    }

    // outline 4 real edges with draw_3d_line which clips them itself
    // outlining the clipped shape would also draw the cut along the near
    // wall, which isnt a real edge of the block
    color outline = shade_colour(fill, 0.6);
    for (int i = 0; i < 4; i++)
    {
        int next = (i + 1) % 4;
        draw_3d_line(cam, c.points[FACE_CORNERS[face][i]], c.points[FACE_CORNERS[face][next]], outline);
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
