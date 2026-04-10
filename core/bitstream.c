#include "bitstream.h"
#include <stdlib.h>

/* =============================
   BIT READER
   ============================= */

void v3d_bitreader_align(v3d_bitreader* br) {
    if (br->bit_pos > 0) {
        br->bit_pos = 0;
        br->byte_pos++;
    }
}

void v3d_bitreader_init(
    v3d_bitreader* br,
    const uint8_t* data,
    size_t size,
    v3d_bit_order order)
{
    br->data = data;
    br->size = size;
    br->byte_pos = 0;
    br->bit_pos = 0;
    br->order = order;
}

uint32_t v3d_read_bits(v3d_bitreader* br, int count)
{
    uint32_t result = 0;

    for (int i = 0; i < count; i++)
    {
        if (br->byte_pos >= br->size)
            return 0;

        uint8_t byte = br->data[br->byte_pos];
        uint8_t bit;

        if (br->order == V3D_BIT_LSB_FIRST)
            bit = (byte >> br->bit_pos) & 1;
        else
            bit = (byte >> (7 - br->bit_pos)) & 1;

        result |= (bit << i);

        br->bit_pos++;

        if (br->bit_pos == 8)
        {
            br->bit_pos = 0;
            br->byte_pos++;
        }
    }

    return result;
}

/* =============================
   BIT WRITER
   ============================= */

int v3d_bitwriter_init(
    v3d_bitwriter* bw,
    size_t capacity)
{
    bw->data = (uint8_t*)malloc(capacity);
    if (!bw->data) return 0;

    bw->capacity = capacity;
    bw->byte_pos = 0;
    bw->current_byte = 0;
    bw->bit_pos = 0;

    return 1;
}

void v3d_write_bits(
    v3d_bitwriter* bw,
    uint32_t value,
    int count)
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
