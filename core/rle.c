#include "rle.h"
#include <stdlib.h>

int v3d_rle_encode(
    const v3d_symbol* input,
    int length,
    v3d_rle_stream* out)
{
    if (!input || length <= 0) return 0;

    v3d_rle_pair* temp =
        (v3d_rle_pair*)malloc(sizeof(v3d_rle_pair) * length);

    int out_index = 0;

    int i = 0;
    while (i < length)
    {
        v3d_symbol current = input[i];
        uint32_t count = 1;

        while (i + count < length &&
               input[i + count] == current)
        {
            count++;
        }

        temp[out_index].symbol = current;
        temp[out_index].count = count;

        out_index++;
        i += count;
    }

    out->data = temp;
    out->size = out_index;

    return 1;
}

void v3d_rle_free(v3d_rle_stream* stream)
{
    free(stream->data);
}
