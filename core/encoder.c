#include "encoder.h"
#include "huffman.h"
#include "bitstream.h"
#include <stdio.h>

/* write only lengths (compact + portable) */
static void v3d_write_huffman_table(
    v3d_bitwriter* bw,
    v3d_huff_code table[256])
{
    for (int i = 0; i < 256; i++)
    {
        v3d_write_bits(bw, table[i].length, 5);
    }
}

extern void v3d_assign_canonical_codes(v3d_huff_code table[256]);

int v3d_full_encode(
    const v3d_rle_stream* rle,
    v3d_bitwriter* bw,
    v3d_huff_code table[256])
{
    // 🔥 IMPORTANT: convert to canonical form
    v3d_assign_canonical_codes(table);

    // 1. write table
    v3d_write_huffman_table(bw, table);

    // 2. write RLE size
    v3d_write_bits(bw, rle->size, 16);

    // 3. encode data
    for (int i = 0; i < rle->size; i++)
    {
        v3d_symbol sym = rle->data[i].symbol;
        v3d_huff_code code = table[sym];

        v3d_write_bits(bw, code.code, code.length);

        // store run-length
        v3d_write_bits(bw, rle->data[i].count, 8);
    }

    return 1;
}
