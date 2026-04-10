#include "gradient.h"
#include <stdlib.h>

int v3d_bitwriter_init(v3d_bitwriter* bw, size_t capacity)
{
    bw->data = (uint8_t*)malloc(capacity);
    if (!bw->data) return 0;

    bw->capacity = capacity;
    bw->byte_pos = 0;
    bw->current_byte = 0;
    bw->bit_pos = 0;

    return 1;
}

void v3d_write_bits(v3d_bitwriter* bw, uint32_t value, int count)
{
    for (int i = 0; i < count; i++)
    {
        uint8_t bit = (value >> i) & 1;

        bw->current_byte |= (bit << bw->bit_pos);

        bw->bit_pos++;

        if (bw->bit_pos == 8)
        {
            bw->data[bw->byte_pos++] = bw->current_byte;
            bw->current_byte = 0;
            bw->bit_pos = 0;
        }
    }
}

void v3d_bitwriter_flush(v3d_bitwriter* bw)
{
    if (bw->bit_pos > 0)
    {
        bw->data[bw->byte_pos++] = bw->current_byte;
        bw->current_byte = 0;
        bw->bit_pos = 0;
    }
}

void v3d_bitwriter_free(v3d_bitwriter* bw)
{
    free(bw->data);
}
