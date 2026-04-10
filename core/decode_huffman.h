#ifndef V3D_DECODE_HUFFMAN_H
#define V3D_DECODE_HUFFMAN_H

#include <stdint.h>

/* =========================
   DECODER NODE
   ========================= */

typedef struct v3d_huff_dnode
{
    int symbol;

    struct v3d_huff_dnode* left;
    struct v3d_huff_dnode* right;

} v3d_huff_dnode;

/* =========================
   NODE HELPERS
   ========================= */

v3d_huff_dnode* v3d_create_dnode(void);
void v3d_free_decoder_tree(v3d_huff_dnode* node);

#endif
