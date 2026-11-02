#include "projection.h"
#include "splashkit.h"
#include <cmath> // sin, cos, tan

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
 * the reverse of world_to_camera, it takes in a point
 * in camera space and then gives where it is in the world.
 * redoes the rotations in the opposite order, pitch first then yaw,
 * with the angles not negated, then adds the camera position back on.
 * used to bring points cut along the players near wall back into the world
 * this is so that they can be drawn using the debug camera.
 */
Vector3 camera_to_world(const Camera &cam, const Vector3 &c)
{
    // redo camera pitch
    double dy = c.y * cos(cam.pitch) - c.z * sin(cam.pitch);
    double rz = c.y * sin(cam.pitch) + c.z * cos(cam.pitch);

    // redo the camera yaw
    double dx = c.x * cos(cam.yaw) - rz * sin(cam.yaw);
    double dz = c.x * sin(cam.yaw) + rz * cos(cam.yaw);

    // back from the position thats relative to the camera to position relative to world
    return {dx + cam.position.x, dy + cam.position.y, dz + cam.position.z};
}

/**
 * returns bool, true if two cameras are in the same exact position facing same direction.
 * true in normal play which lets draw_face skip the extra player-side clipping
 */
bool cameras_match(const Camera &a, const Camera &b)
{
    return a.position.x == b.position.x && a.position.y == b.position.y &&
           a.position.z == b.position.z && a.yaw == b.yaw && a.pitch == b.pitch;
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

/**
 * cuts a polygon given in world space along camera's near wall,
 * gives back the part in front of near wall, in world space.
 * clip_polygon_to_near_plane only works in camera space.
 * when drawing from debug camera, this function is used to cut polygons
 * around player near wall rather than the viewing camera near wall
 */
Polygon clip_world_polygon_to_near_plane(const Camera &cam, const Polygon &world_shape)
{
    Polygon cam_shape;
    cam_shape.size = world_shape.size;
    for (int i = 0; i < world_shape.size; i++)
    {
        cam_shape.points[i] = world_to_camera(cam, world_shape.points[i]);
    }

    Polygon clipped = clip_polygon_to_near_plane(cam_shape);

    Polygon result;
    result.size = clipped.size;
    for (int i = 0; i < clipped.size; i++)
    {
        result.points[i] = camera_to_world(cam, clipped.points[i]);
    }

    return result;
}

/**
 * same as clip_world_polygon_to_near_plane but for a line, changes start and end in place
 * returns false if the whole line is behind the wall
 */
bool clip_world_line_to_near_plane(const Camera &cam, Vector3 &start, Vector3 &end)
{
    Vector3 a = world_to_camera(cam, start);
    Vector3 b = world_to_camera(cam, end);

    bool a_in_front = a.z >= NEAR_PLANE;
    bool b_in_front = b.z >= NEAR_PLANE;

    if (!a_in_front && !b_in_front)
    {
        return false;
    }

    // same as draw_3d_line: slide the end thats behind the wall to the near plane
    if (!a_in_front)
    {
        a = near_plane_intersection(a, b);
    }
    if (!b_in_front)
    {
        b = near_plane_intersection(a, b);
    }

    start = camera_to_world(cam, a);
    end = camera_to_world(cam, b);
    return true;
}

// is this pixel on the window side of the given edge?
bool is_inside_edge(const Vector2 &p, ScreenEdge edge)
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

        bool current_inside = is_inside_edge(current, edge);
        bool next_inside = is_inside_edge(next, edge);

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
// LINES
// ============================================================

/**
 * a line between two points in the world, cut at the near wall first
 * so an end behind the camera doesn't flip to the wrong side of the screen
 */
void draw_3d_line(const Camera &cam, const Vector3 &start, const Vector3 &end, color line_color)
{
    Vector3 a = world_to_camera(cam, start);
    Vector3 b = world_to_camera(cam, end);

    // >= not > because a clipped end sits exactly on the wall and must count
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

    // both ends are now in front, so the divide in camera_to_screen is safe
    Vector2 screen_start = camera_to_screen(cam, a);
    Vector2 screen_end = camera_to_screen(cam, b);

    draw_line(line_color, screen_start.x, screen_start.y, screen_end.x, screen_end.y);
}
