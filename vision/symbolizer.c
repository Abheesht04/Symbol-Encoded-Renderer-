#include "symbolizer.h"
#include <stdlib.h>

void v3d_symbolize(
    uint8_t* pixels,
    int width,
    int height,
    v3d_geom_symbol* out)
{
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int i = y * width + x;

            int gx =
                abs(pixels[i] - pixels[i+1]) +
                abs(pixels[i] - pixels[i-1]);

            int gy =
                abs(pixels[i] - pixels[i+width]) +
                abs(pixels[i] - pixels[i-width]);

            int g = gx + gy;

            if (g > 30)
                out[i] = V3D_EDGE;
            else if (g > 15)
                out[i] = V3D_LINE;
            else if (g > 5)
                out[i] = V3D_SURFACE;
            else
                out[i] = V3D_EMPTY;
        }
    }
}
