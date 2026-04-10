#include "geometry.h"
#include "render_types.h"

int v3d_symbols_to_lines(
    v3d_geom_symbol* data,
    int size,
    v3d_line* lines)
{
    int line_count = 0;

    for (int i = 0; i < size; i++)
    {
        if (data[i] == V3D_LINE)
        {
            v3d_line l;

            l.vertices[0].position = (v3d_vec3){i, 0, 0};
            l.vertices[1].position = (v3d_vec3){i, 1, 0};

            lines[line_count++] = l;
        }
    }

    return line_count;
}
