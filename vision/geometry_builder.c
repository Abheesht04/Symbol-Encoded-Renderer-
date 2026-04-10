#include "geometry.h"

/*
   Converts symbol grid → line geometry
*/
int v3d_build_geometry(
    v3d_geom_symbol* data,
    int width,
    int height,
    v3d_vertex* out)
{
    int v = 0;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int i = y * width + x;

            float fx = (float)x / width * 2.0f - 1.0f;
            float fy = (float)y / height * 2.0f - 1.0f;

            switch (data[i])
            {
                case V3D_EDGE:
                {
                    // small horizontal line
                    out[v++] = (v3d_vertex){fx, fy, 0, 1,1,1,1};
                    out[v++] = (v3d_vertex){fx+0.003f, fy, 0, 1,1,1,1};
                    break;
                }

                case V3D_LINE:
                {
                    // longer line
                    out[v++] = (v3d_vertex){fx, fy, 0, 0,1,0,1};
                    out[v++] = (v3d_vertex){fx+0.02f, fy, 0, 0,1,0,1};
                    break;
                }

                case V3D_CURVE:
                {
                    // simple curve representation (diagonal)
                    out[v++] = (v3d_vertex){fx, fy, 0, 1,0,0,1};
                    out[v++] = (v3d_vertex){fx+0.02f, fy+0.02f, 0, 1,0,0,1};
                    break;
                }

                case V3D_SURFACE:
                {
                    // tiny cross to visualize surface
                    out[v++] = (v3d_vertex){fx, fy, 0, 0,0,1,1};
                    out[v++] = (v3d_vertex){fx+0.01f, fy, 0, 0,0,1,1};

                    out[v++] = (v3d_vertex){fx, fy, 0, 0,0,1,1};
                    out[v++] = (v3d_vertex){fx, fy+0.01f, 0, 0,0,1,1};
                    break;
                }

                default:
                    break;
            }
        }
    }

    return v;
}
