#include "huffman.h"
#include <string.h>

static uint32_t reverse_bits(uint32_t code, int length)
{
    uint32_t result = 0;

    for (int i = 0; i < length; i++)
    {
        result <<= 1;
        result |= (code & 1);
        code >>= 1;
    }

    return result;
}

/* Assign canonical codes from lengths */
void v3d_assign_canonical_codes(v3d_huff_code table[256])
{
    int bl_count[32] = {0};
    int next_code[32] = {0};

    for (int i = 0; i < 256; i++)
        if (table[i].length > 0)
            bl_count[table[i].length]++;

    int code = 0;

    for (int bits = 1; bits < 32; bits++)
    {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    for (int i = 0; i < 256; i++)
    {
        int len = table[i].length;

        if (len > 0)
        {
            uint32_t c = next_code[len];
            next_code[len]++;

	    table[i].code = reverse_bits(c,len);
        }
    }
}
