#include "png_process.h"
#include <stdint.h>
#include <stdlib.h>
static int paeth(int a, int b, int c)
{
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);

    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

void v3d_png_unfilter(
    uint8_t* data,
    int width,
    int height,
    int bpp)
{
    int stride = width * bpp;

    for (int y = 0; y < height; y++)
    {
        uint8_t filter = data[y * (stride + 1)];
        uint8_t* row = &data[y * (stride + 1) + 1];
        uint8_t* prev = (y > 0) ? &data[(y - 1)*(stride+1) + 1] : NULL;

        switch (filter)
        {
            case 0: break;

            case 1: // Sub
                for (int x = bpp; x < stride; x++)
                    row[x] += row[x - bpp];
                break;

            case 2: // Up
                if (prev)
                    for (int x = 0; x < stride; x++)
                        row[x] += prev[x];
                break;

            case 3: // Average
                for (int x = 0; x < stride; x++)
                {
                    int left = (x >= bpp) ? row[x - bpp] : 0;
                    int up = prev ? prev[x] : 0;
                    row[x] += (left + up) / 2;
                }
                break;

            case 4: // Paeth
                for (int x = 0; x < stride; x++)
                {
                    int left = (x >= bpp) ? row[x - bpp] : 0;
                    int up = prev ? prev[x] : 0;
                    int up_left = (prev && x >= bpp) ? prev[x - bpp] : 0;
                    row[x] += paeth(left, up, up_left);
                }
                break;
        }
    }
}
