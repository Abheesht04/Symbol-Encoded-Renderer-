#include "geometry.h"

/*
    Convert symbol grid → renderable line geometry
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

            // Normalize to [-1, 1]
            float fx = ((float)x / (float)(width - 1)) * 2.0f - 1.0f;
            float fy = ((float)y / (float)(height - 1)) * 2.0f - 1.0f;

            // Flip Y for screen-style coords (optional)
            fy = -fy;

            switch (data[i])
            {
                case V3D_EDGE:
                {
                    out[v++] = (v3d_vertex){fx, fy, 0, 1,1,1,1};
                    out[v++] = (v3d_vertex){fx + 0.002f, fy, 0, 1,1,1,1};
                    break;
                }

                case V3D_LINE:
                {
                    out[v++] = (v3d_vertex){fx, fy, 0, 0,1,0,1};
                    out[v++] = (v3d_vertex){fx + 0.02f, fy, 0, 0,1,0,1};
                    break;
                }

                case V3D_CURVE:
                {
                    out[v++] = (v3d_vertex){fx, fy, 0, 1,0,0,1};
                    out[v++] = (v3d_vertex){fx + 0.02f, fy + 0.02f, 0, 1,0,0,1};
                    break;
                }

                case V3D_SURFACE:
                {
                    // Cross
                    out[v++] = (v3d_vertex){fx, fy, 0, 0,0,1,1};
                    out[v++] = (v3d_vertex){fx + 0.01f, fy, 0, 0,0,1,1};

                    out[v++] = (v3d_vertex){fx, fy, 0, 0,0,1,1};
                    out[v++] = (v3d_vertex){fx, fy + 0.01f, 0, 0,0,1,1};
                    break;
                }

                case V3D_CORNER:
                {
                    // L shape
                    out[v++] = (v3d_vertex){fx, fy, 0, 1,1,0,1};
                    out[v++] = (v3d_vertex){fx + 0.01f, fy, 0, 1,1,0,1};

                    out[v++] = (v3d_vertex){fx, fy, 0, 1,1,0,1};
                    out[v++] = (v3d_vertex){fx, fy + 0.01f, 0, 1,1,0,1};
                    break;
                }

                default:
                    break;
            }
        }
    }

    return v;
}
