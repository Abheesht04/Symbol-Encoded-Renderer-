#ifndef V3D_RENDER_TYPES_H
#define V3D_RENDER_TYPES_H

typedef struct
{
    float x, y, z;
} v3d_vec3;

typedef struct
{
    v3d_vec3 position;
} v3d_vertex;

typedef struct
{
    v3d_vertex vertices[2]; // line = 2 points
} v3d_line;

#endif
