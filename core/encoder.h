#ifndef V3D_ENCODER_H
#define V3D_ENCODER_H

#include "rle.h"
#include "bitstream.h"
#include "huffman.h"

int v3d_full_encode(
    const v3d_rle_stream* rle,
    v3d_bitwriter* bw,
    v3d_huff_code table[256]);

void v3d_assign_canonical_codes(v3d_huff_code table[256]);

#endif
