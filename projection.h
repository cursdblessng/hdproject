// camera maths: turning a point in the world into a pixel on the screen, and cutting off
// anything behind the camera or outside the window before it's drawn
#pragma once

#include "world.h"

// how far in front of the camera the "near wall" is
// anything closer than this gets cut off, because dividing by a z
// thats tiny or negative blows up or flips the point
const double NEAR_PLANE = 0.1;

// a flat shape with a variable number of corners, used for clipping
// each straight cut can add at most 1 corner: 4 corners, +1 from the
// player's near wall (debug view only), +1 from the view camera's near
// wall, +1 from each of the 4 screen edges = 10 at most
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

// step 1: moves and rotates a world point so the camera is at (0,0,0) looking down +z
Vector3 world_to_camera(const Camera &cam, const Vector3 &p);

// the reverse of world_to_camera, a camera space point back into the world
Vector3 camera_to_world(const Camera &cam, const Vector3 &c);

// same position and direction. true in normal play, when the view camera is the player
bool cameras_match(const Camera &a, const Camera &b);

// step 2: the pixel a camera space point lands on. only for points in front of the near wall
Vector2 camera_to_screen(const Camera &cam, const Vector3 &c);

// the part of a camera space polygon in front of the near wall
Polygon clip_polygon_to_near_plane(const Polygon &input);

// the same for a polygon in world space, cut along this camera's near wall
Polygon clip_world_polygon_to_near_plane(const Camera &cam, const Polygon &world_shape);

// the same for a line, changes start and end. false if the whole line is behind the wall
bool clip_world_line_to_near_plane(const Camera &cam, Vector3 &start, Vector3 &end);

// the part of a screen polygon inside the window
ScreenPolygon clip_polygon_to_screen(const ScreenPolygon &input);

// a line between two points in the world, cut at the near wall first
void draw_3d_line(const Camera &cam, const Vector3 &start, const Vector3 &end, color line_color);
