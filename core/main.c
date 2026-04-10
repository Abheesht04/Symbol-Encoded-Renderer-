#include "rle.h"
#include "huffman.h"
#include "bitstream.h"
#include "encoder.h"
#include "decode_huffman.h"
#include "decoder.h"
#include <stdio.h>

int main()
{
    v3d_symbol input[] = {
        0,0,0,1,1,2,2,2,2
    };

    int length = sizeof(input);

    // -------------------
    // RLE
    // -------------------
    v3d_rle_stream rle;
    v3d_rle_encode(input, length, &rle);

    // -------------------
    // HUFFMAN
    // -------------------
    v3d_symbol sym_stream[256];
    for (int i = 0; i < rle.size; i++)
        sym_stream[i] = rle.data[i].symbol;

    v3d_huff_code table[256];
    v3d_build_huffman(sym_stream, rle.size, table);

    // -------------------
    // ENCODE
    // -------------------
    v3d_bitwriter bw;
    v3d_bitwriter_init(&bw, 100);

    v3d_full_encode(&rle, &bw, table);
    v3d_bitwriter_flush(&bw);

    // -------------------
    // BUILD DECODER TREE
    // -------------------
    v3d_huff_dnode* tree =
        v3d_build_decoder_tree(table);

    // -------------------
    // DECODE
    // -------------------
    v3d_bitreader br;
    v3d_bitreader_init(&br, bw.data, bw.byte_pos, V3D_BIT_LSB_FIRST);

    v3d_symbol output[256];
    int out_len = 0;

    v3d_full_decode(&br, tree, output, &out_len);

    // -------------------
    // PRINT RESULT
    // -------------------
    printf("Decoded output:\n");

    for (int i = 0; i < out_len; i++)
        printf("%d ", output[i]);

    printf("\n");

    // cleanup
    v3d_free_decoder_tree(tree);
    v3d_rle_free(&rle);
    v3d_bitwriter_free(&bw);

    return 0;
}
