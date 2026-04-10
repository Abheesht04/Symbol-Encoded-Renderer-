#ifndef V3D_GEOMETRY_H
#define V3D_GEOMETRY_H

typedef enum{
    V3D_EMPTY = 0,
    V3D_EDGE  = 1,
    V3D_LINE  = 2,
    V3D_CURVE = 3,
    V3D_SURFACE = 4,
    V3D_CORNER = 5,
} v3d_geom_symbol;

/* GPU vertex */
typedef struct {
    float x,y,z;
    float r,g,b,a;
} v3d_vertex;

/* build geometry */
int v3d_build_geometry(
    v3d_geom_symbol* data,
    int width,
    int height,
    v3d_vertex* out_vertices);

#endif
