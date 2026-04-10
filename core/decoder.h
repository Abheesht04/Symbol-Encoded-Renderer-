#ifndef V3D_DECODER_H
#define V3D_DECODER_H

#include "bitstream.h"
#include "decode_huffman.h"
#include "huffman.h"

/* =========================
   FULL PIPELINE DECODE
   ========================= */

/*
    Decodes full compressed stream:

    - Reads Huffman table (code lengths)
    - Rebuilds canonical codes
    - Builds decoding tree
    - Decodes RLE stream
    - Expands into output symbols
*/
int v3d_full_decode(
    v3d_bitreader* br,
    v3d_symbol* output,
    int* out_len);

#endif
