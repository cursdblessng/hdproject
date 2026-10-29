// drawing the world: projection, clipping, wireframe and solid blocks
#pragma once

#include "world.h"

// grey ground grid on y = 0, grid_radius lines out from the origin
void draw_grid(const Camera &cam, int grid_radius);

// every active block as 12 coloured edges (handy for debugging)
void draw_world_wireframe(const Camera &cam, const BlockArray &world);

// every active block as shaded solid faces (painter's algorithm)
void draw_world_solid(const Camera &cam, const BlockArray &world);
