#ifndef V3D_HUFFMAN_H
#define V3D_HUFFMAN_H

#include "symbol.h"

typedef struct v3d_huff_node {
    v3d_symbol symbol;
    int freq;

    struct v3d_huff_node* left;
    struct v3d_huff_node* right;
} v3d_huff_node;

typedef struct {
    uint32_t code;
    uint8_t length;
} v3d_huff_code;

int v3d_build_huffman(
    const v3d_symbol* data,
    int length,
    v3d_huff_code table[256]);

#endif
