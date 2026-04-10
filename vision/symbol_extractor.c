#include <stdlib.h>
#include <stdint.h>
void extract_edges(
    uint8_t* img,
    int width,
    int height,
    uint8_t* out)
{
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int i = y * width + x;

            int gx = abs(img[i] - img[i+1]) +
                     abs(img[i] - img[i-1]);

            int gy = abs(img[i] - img[i+width]) +
                     abs(img[i] - img[i-width]);

            int g = gx + gy;

            out[i] = (g > 40) ? 1 : 0;
        }
    }
}
