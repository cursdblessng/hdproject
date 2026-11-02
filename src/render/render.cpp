#include "render.h"
#include "projection.h"
#include "sky.h" // block_light
#include "splashkit.h"
#include <cmath>     // sin, cos, tan, sqrt, floor
#include <algorithm> // std::sort
#include <iterator>  // std::begin

// ============================================================
// CONSTANTS AND STRUCTS
// ============================================================

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

// how solid faces are in see-through mode: 0 = invisible, 255 = solid.
// 128 is about half, enough to see the faces behind but still tell them apart
const int SEE_THROUGH_ALPHA = 128;

/**
 * one visible face waiting to be drawn
 * stores which block and which face instead of the projected points
 * to keep it small (24 bytes) so the list fits on the stack
 */
struct FaceRef
{
    int x, y, z;  // coords of the block this face belongs to
    int face;     // 0 to 5, index into FACE_CORNERS
    double depth; // squared distance from camera, for sorting
};

// how many blocks out are rendered around the player, in every direction (a cube, not a sphere)
const int RENDER_DISTANCE = 32;

// distance from a block's centre to its furthest corner, half of the cube's
// diagonal: sqrt(0.5² + 0.5² + 0.5²) = sqrt(3) / 2, about 0.866.
const double BLOCK_RADIUS = sqrt(3.0) / 2.0;

struct FaceList
{
    FaceRef data[MAX_VISIBLE_FACES];
    int size;
};

/**
 * the pyramid shape a camera can see, with the camera at its tip.
 * stored as the camera's position and directions plus how slanted the side walls are
 */
struct Frustum
{
    Vector3 position; // camera position
    Vector3 right;    // camera space +x
    Vector3 up;       // camera space +y
    Vector3 forward;  // camera space +z
    double slope_x;   // left/right screen edges
    double slope_y;   // top/bottom screen edges
    double margin_x;  // how far past left/right wall a block's centre can be and still poke through wall
    double margin_y;  // same for top and bottom
};

// ============================================================
// FRUSTUM CULLING
// ============================================================

/**
 * the frustum for frustum culling: blocks completely outside the camera's view are skipped
 * before any of their faces are projected. screen clipping would hide them anyway,
 * but only after all the work of projecting them.
 * right, up and forward are the camera's directions in the world,
 * the same rotations world_to_camera undoes
 */
Frustum make_frustum(const Camera &cam)
{
    Frustum f;
    f.position = cam.position;

    double sin_yaw = sin(cam.yaw);
    double cos_yaw = cos(cam.yaw);
    double sin_pitch = sin(cam.pitch);
    double cos_pitch = cos(cam.pitch);

    f.right = {cos_yaw, 0.0, sin_yaw};
    f.up = {-sin_yaw * sin_pitch, cos_pitch, cos_yaw * sin_pitch};
    f.forward = {-sin_yaw * cos_pitch, -sin_pitch, cos_yaw * cos_pitch};

    f.slope_x = tan(cam.fov / 2.0);

    // the screen is shorter than it is wide, so the top/bottom edges are
    // closer to the middle, same slope scaled down by height / width.
    // has to be set before margin_y below, which uses it
    f.slope_y = f.slope_x * screen_height() / (double)screen_width();

    // the walls are slanted, so a ball's centre has to be further past the wall
    // measured along x (or y) than its radius for the whole ball to be past it.
    // along-x distance = right-angle distance * sqrt(1 + slope^2)
    // (slope 0 = flat wall, sqrt(1) = no change. slope 1 = 45 degrees, sqrt(2))
    f.margin_x = BLOCK_RADIUS * sqrt(1.0 + f.slope_x * f.slope_x);
    f.margin_y = BLOCK_RADIUS * sqrt(1.0 + f.slope_y * f.slope_y);

    return f;
}

// how far a goes along b, when b is length 1
double dot_product(const Vector3 &a, const Vector3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
 * could any part of the block be inside the frustum at all
 * the block is treated as a ball around its centre (BLOCK_RADIUS covers its corners),
 * so a block only just poking into the view is still kept
 * finds the centre's camera space coords, then checks it against all 5 walls of the frustum
 */
bool block_in_frustum(const Frustum &f, int x, int y, int z)
{
    // offset from the camera to the block's centre
    // +0.5 because xyz is the block's min corner not the middle
    Vector3 offset = {x + 0.5 - f.position.x, y + 0.5 - f.position.y, z + 0.5 - f.position.z};

    double cam_x = dot_product(offset, f.right);   // how far right of the camera
    double cam_y = dot_product(offset, f.up);      // how far above
    double cam_z = dot_product(offset, f.forward); // how far in front

    // behind the near wall, even the closest edge of the ball.
    // checked first because about half of all blocks are behind
    if (cam_z < NEAR_PLANE - BLOCK_RADIUS)
    {
        return false;
    }

    // past the right wall: further right than z * slope_x, by more than the margin
    if (cam_x - cam_z * f.slope_x > f.margin_x)
    {
        return false;
    }

    // past the left wall: same test mirrored, -x instead of x
    if (-cam_x - cam_z * f.slope_x > f.margin_x)
    {
        return false;
    }

    // past the top wall
    if (cam_y - cam_z * f.slope_y > f.margin_y)
    {
        return false;
    }

    // past the bottom wall
    if (-cam_y - cam_z * f.slope_y > f.margin_y)
    {
        return false;
    }

    // not completely past any wall, so (some of) it might be on screen
    return true;
}

/**
 * shows frustum culling from the debug camera: 4 lines from the player's eye out along
 * the corners of their view, joined at the far end into a rectangle.
 * blocks outside these lines are the ones frustum culling skips.
 * each corner direction is forward plus the slopes along right and up, which is
 * exactly the slanted walls block_in_frustum tests against.
 * the far end is RENDER_DISTANCE ahead, roughly where blocks stop being drawn
 */
void draw_player_frustum(const Camera &view_cam, const Camera &player_cam)
{
    Frustum f = make_frustum(player_cam);
    double distance = RENDER_DISTANCE;

    // which side each corner is on: top left, top right, bottom right, bottom left.
    // going round in order so joining each one to the next draws the rectangle
    const double SIDE_X[4] = {-1.0, 1.0, 1.0, -1.0};
    const double SIDE_Y[4] = {1.0, 1.0, -1.0, -1.0};

    Vector3 far_corners[4];
    for (int i = 0; i < 4; i++)
    {
        double right_amount = SIDE_X[i] * f.slope_x;
        double up_amount = SIDE_Y[i] * f.slope_y;

        far_corners[i] = {f.position.x + (f.forward.x + f.right.x * right_amount + f.up.x * up_amount) * distance,
                          f.position.y + (f.forward.y + f.right.y * right_amount + f.up.y * up_amount) * distance,
                          f.position.z + (f.forward.z + f.right.z * right_amount + f.up.z * up_amount) * distance};
    }

    for (int i = 0; i < 4; i++)
    {
        draw_3d_line(view_cam, f.position, far_corners[i], COLOR_RED);
        draw_3d_line(view_cam, far_corners[i], far_corners[(i + 1) % 4], COLOR_RED);
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
 * back face culling
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
 * hidden face culling
 * a face is hidden if the cell it points into covers it:
 * a solid block always covers it
 * the same type also covers it, so there are no faces between water blocks
 * water doesnt cover other blocks since it's see through, so the sea floor still shows
 */
bool is_face_hidden(const World &world, int x, int y, int z, int face)
{
    Vector3 normal = FACE_NORMALS[face];

    // normals are only ever 0 1 or -1
    int neighbour_x = x + (int)normal.x;
    int neighbour_y = y + (int)normal.y;
    int neighbour_z = z + (int)normal.z;

    BlockType neighbour = get_block(world, neighbour_x, neighbour_y, neighbour_z);

    return is_solid(neighbour) || neighbour == get_block(world, x, y, z);
}

/**
 * squared distance between two points, used for sorting faces.
 * skips the sqrt: if a > b then a squared > b squared, so the order comes out the same
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
    return rgba_color((int)(red_of(base) * factor),
                      (int)(green_of(base) * factor),
                      (int)(blue_of(base) * factor),
                      alpha_of(base));
}

/**
 * fills the list with every face the player could see: blocks within RENDER_DISTANCE
 * and inside the frustum, faces not covered by a neighbour and facing the camera.
 * culling uses cull_cam (always the player), depth uses view_cam,
 * so the debug camera sorts by its own distance but only sees what the player would draw.
 * stops early if the list fills up
 */
void collect_visible_faces(const Camera &view_cam, const Camera &cull_cam, const World &world, const Settings &settings, FaceList &faces)
{
    faces.size = 0;

    // each kind of culling can be turned off in the debug settings to show what it saves
    bool use_frustum_culling = is_on(settings, SETTING_FRUSTUM_CULLING);
    bool use_back_face_culling = is_on(settings, SETTING_BACK_FACE_CULLING);
    bool use_hidden_face_culling = is_on(settings, SETTING_HIDDEN_FACE_CULLING);

    Frustum frustum = make_frustum(cull_cam);

    // cell player's eyes are in
    int centre_x = (int)floor(cull_cam.position.x);
    int centre_y = (int)floor(cull_cam.position.y);
    int centre_z = (int)floor(cull_cam.position.z);

    for (int x = centre_x - RENDER_DISTANCE; x <= centre_x + RENDER_DISTANCE; x++)
    {
        for (int y = centre_y - RENDER_DISTANCE; y <= centre_y + RENDER_DISTANCE; y++)
        {
            for (int z = centre_z - RENDER_DISTANCE; z <= centre_z + RENDER_DISTANCE; z++)
            {
                BlockType type = get_block(world, x, y, z);

                // empty cell, nothing to draw
                if (type == AIR)
                {
                    continue;
                }

                // whole block is outside the player's view, so skip it before
                // building its corners, testing its faces, or adding to the sort
                if (use_frustum_culling && !block_in_frustum(frustum, x, y, z))
                {
                    continue;
                }

                // you can be inside water and see through it, so its surface has to show
                // from below too. back face culling would throw it away when you're underwater
                bool is_double_sided = type == WATER;

                CubeCorners c = get_block_corners(x, y, z);

                for (int f = 0; f < NUM_FACES; f++)
                {
                    // checked before back face culling because its only one array lookup
                    if (use_hidden_face_culling && is_face_hidden(world, x, y, z, f))
                    {
                        continue;
                    }

                    // with back face culling off every face counts as facing the camera
                    if (is_double_sided || !use_back_face_culling || is_face_visible(cull_cam, c, f))
                    {
                        // list full, so stop adding
                        if (faces.size >= MAX_VISIBLE_FACES)
                        {
                            return;
                        }

                        faces.data[faces.size] = {x, y, z, f, distance_squared(view_cam.position, face_centre(c, f))};
                        faces.size++;
                    }
                }
            }
        }
    }
}

/**
 * the rule std::sort uses to order the faces: should face a come before face b?
 * > (further first) because the painter's algorithm draws far to near.
 * must be > and NOT >=: std::sort needs "a before b" to be false when two
 * faces are the same distance, otherwise it can read past the end of the array
 * and crash. the superflat has lots of faces at exactly equal distances
 */
bool is_further(const FaceRef &a, const FaceRef &b)
{
    return a.depth > b.depth;
}

/**
 * painter's algorithm needs furthest faces first.
 * std::sort (introsort: mostly quicksort) takes about n * log2(n) steps,
 * instead of insertion sort's n * n. for 10,000 faces that's ~0.65 ms instead of ~37 ms.
 * it sorts in place, so no second 240 KB array is needed on the stack.
 * std::begin(faces.data) is the start of the array, and + faces.size is
 * where the used part ends, so the unused slots after that are left alone
 */
void sort_faces_far_to_close(FaceList &faces)
{
    std::sort(std::begin(faces.data), std::begin(faces.data) + faces.size, is_further);
}

/**
 * projects one face to the screen and fills it, cutting off any part behind the near wall
 * and outside the window. light darkens it for the time of day,
 * see_through draws it at half opacity so faces behind it show,
 * show_outlines draws its 4 edges on top
 */
void draw_face(const Camera &view_cam, const Camera &cull_cam, int x, int y, int z, BlockType type, int face, bool see_through, double light, bool show_outlines, FrameTimer &timer)
{
    // everything up to the fill is our own maths, timed as project
    double project_start = time_now_ms();

    CubeCorners c = get_block_corners(x, y, z);

    // the face's 4 corners, still in world space
    Polygon world_shape;
    world_shape.size = 4;
    for (int i = 0; i < 4; i++)
    {
        world_shape.points[i] = c.points[FACE_CORNERS[face][i]];
    }

    // debug camera only: first cut away whatever is behind the player's near
    // wall, so the debug view shows exactly what the player actually draws.
    // skipped in normal play, where both cameras are the player and the
    // view camera's own cut below already does this
    bool is_debug_view = !cameras_match(view_cam, cull_cam);
    if (is_debug_view)
    {
        world_shape = clip_world_polygon_to_near_plane(cull_cam, world_shape);

        // whole face is behind the player, the player doesn't draw it
        if (world_shape.size < 3)
        {
            return;
        }
    }

    // projection step 1 for the face's corners, from the view camera.
    // stays in camera space so clipping can see each corner's depth
    Polygon face_shape;
    face_shape.size = world_shape.size;
    for (int i = 0; i < world_shape.size; i++)
    {
        face_shape.points[i] = world_to_camera(view_cam, world_shape.points[i]);
    }

    // cut off whatever is behind the near wall
    Polygon clipped = clip_polygon_to_near_plane(face_shape);

    // whole face was behind the wall, nothing left to fill
    if (clipped.size < 3)
    {
        return;
    }

    // projection step 2: every remaining corner is in front of the wall, so the divide is safe
    ScreenPolygon screen_shape;
    screen_shape.size = clipped.size;
    for (int i = 0; i < clipped.size; i++)
    {
        screen_shape.points[i] = camera_to_screen(view_cam, clipped.points[i]);
    }

    // corners close to the camera can land tens of thousands of pixels
    // off screen, which overflows SplashKit's triangle filling (it uses
    // 16-bit numbers, max 32767), which causes lag and random shapes.
    // cutting to the window keeps every corner inside it
    ScreenPolygon on_screen = clip_polygon_to_screen(screen_shape);

    // face is entirely off the side of the window, so dont render
    if (on_screen.size < 3)
    {
        return;
    }

    // light darkens every face the same amount at night, on top of each side's own shade
    color fill = shade_colour(block_colour(type), FACE_SHADE[face] * light);

    // same colour, but with the alpha lowered so faces behind show through
    // works because faces are drawn far to near, so each one
    // blends with whatever is already drawn behind it
    if (see_through)
    {
        fill = rgba_color(red_of(fill), green_of(fill), blue_of(fill), SEE_THROUGH_ALPHA);
    }

    add_stage_time(timer, STAGE_PROJECT, project_start);

    double fill_start = time_now_ms();

    // triangle fan: every triangle shares corner 0
    // starts at 1, starting with 0 would give 0,0,1, a triangle with no area
    for (int i = 1; i < on_screen.size - 1; i++)
    {
        fill_triangle(fill,
                      on_screen.points[0].x, on_screen.points[0].y,
                      on_screen.points[i].x, on_screen.points[i].y,
                      on_screen.points[i + 1].x, on_screen.points[i + 1].y);
    }
    add_stage_time(timer, STAGE_FILL, fill_start);

    double outline_start = time_now_ms();

    // outline 4 real edges with draw_3d_line which clips them itself
    // outlining the clipped shape would also draw the cut along the near
    // wall, which isnt a real edge of the block.
    // shade_colour keeps fill's alpha, so outlines go see-through too
    color outline = shade_colour(fill, 0.6);

    // off by default because drawing 4 lines per face is slow
    if (show_outlines)
    {
        for (int i = 0; i < 4; i++)
        {
            int next = (i + 1) % 4;
            Vector3 start = c.points[FACE_CORNERS[face][i]];
            Vector3 end = c.points[FACE_CORNERS[face][next]];

            // debug camera: cut the edge along the player's near wall too, so
            // the outline matches the cut face. skip it if it's all behind
            if (is_debug_view && !clip_world_line_to_near_plane(cull_cam, start, end))
            {
                continue;
            }

            draw_3d_line(view_cam, start, end, outline);
        }
    }

    add_stage_time(timer, STAGE_OUTLINE, outline_start);
}

// collect then sort then draw far to near, returns how many faces there were for the debug info
int draw_world_solid(const Camera &view_cam, const Camera &cull_cam, const World &world, bool see_through, double time_of_day, const Settings &settings, FrameTimer &timer)
{
    // the same for every face this frame, so it's only worked out once
    double light = block_light(time_of_day);
    bool show_outlines = is_on(settings, SETTING_OUTLINES);

    // made fresh every frame because the camera moves, so which faces are visible and their order changes each frame
    FaceList faces;

    double collect_start = time_now_ms();
    collect_visible_faces(view_cam, cull_cam, world, settings, faces);
    add_stage_time(timer, STAGE_COLLECT, collect_start);

    double sort_start = time_now_ms();
    sort_faces_far_to_close(faces);
    add_stage_time(timer, STAGE_SORT, sort_start);

    // if this sits at MAX_VISIBLE_FACES, faces are being dropped
    timer.faces += faces.size;

    for (int i = 0; i < faces.size; i++)
    {
        FaceRef f = faces.data[i];
        // look the type up here: the cell must still hold a block, because
        // nothing changes the world between collecting and drawing
        draw_face(view_cam, cull_cam, f.x, f.y, f.z, get_block(world, f.x, f.y, f.z), f.face, see_through, light, show_outlines, timer);
    }

    return faces.size;
}

/**
 * tints the whole screen blue when the camera's eye is inside water
 * drawn after the world so it covers everything, but before the hud so that stays clear
 * the cell the eye is in is found the same way as everywhere else, floor of the position
 */
void draw_underwater_tint(const Camera &cam, const World &world)
{
    int eye_x = (int)floor(cam.position.x);
    int eye_y = (int)floor(cam.position.y);
    int eye_z = (int)floor(cam.position.z);

    if (get_block(world, eye_x, eye_y, eye_z) != WATER)
    {
        return;
    }

    // low alpha so you can still see through it, just shifted towards blue
    fill_rectangle(rgba_color(30, 60, 160, 90), 0, 0, screen_width(), screen_height());
}
