#include "decode_huffman.h"
#include <stdlib.h>

/* =========================
   CREATE NODE
   ========================= */

v3d_huff_dnode* v3d_create_dnode(void)
{
    v3d_huff_dnode* n =
        (v3d_huff_dnode*)malloc(sizeof(v3d_huff_dnode));

    n->symbol = -1;
    n->left = NULL;
    n->right = NULL;

    return n;
}

/* =========================
   FREE TREE
   ========================= */

void v3d_free_decoder_tree(v3d_huff_dnode* node)
{
    if (!node) return;

    v3d_free_decoder_tree(node->left);
    v3d_free_decoder_tree(node->right);

    free(node);
}
