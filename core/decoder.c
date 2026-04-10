#include "decoder.h"
#include "decode_huffman.h"
#include "huffman.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* =========================
   READ HUFFMAN TABLE (lengths)
   ========================= */

static void v3d_read_huffman_table(
    v3d_bitreader* br,
    v3d_huff_code table[256])
{
    for (int i = 0; i < 256; i++)
    {
        table[i].length = v3d_read_bits(br, 5);
        table[i].code = 0;
    }
}

/* =========================
   CANONICAL CODE ASSIGNMENT
   ========================= */

void v3d_assign_canonical_codes(v3d_huff_code table[256]);

/* =========================
   BUILD DECODER TREE
   ========================= */

static v3d_huff_dnode* build_tree(
    v3d_huff_code table[256])
{
    v3d_huff_dnode* root = v3d_create_dnode();

    for (int i = 0; i < 256; i++)
    {
        if (table[i].length == 0)
            continue;

        v3d_huff_dnode* cur = root;

        for (int b = 0; b < table[i].length; b++)
        {
            int bit = (table[i].code >> b) & 1;

            if (bit == 0)
            {
                if (!cur->left)
                    cur->left = v3d_create_dnode();
                cur = cur->left;
            }
            else
            {
                if (!cur->right)
                    cur->right = v3d_create_dnode();
                cur = cur->right;
            }
        }

        cur->symbol = i;
    }

    return root;
}

/* =========================
   FULL DECODE PIPELINE
   ========================= */

int v3d_full_decode(
    v3d_bitreader* br,
    v3d_symbol* output,
    int* out_len)
{
    v3d_huff_code table[256];

    // 1. read Huffman table (lengths)
    v3d_read_huffman_table(br, table);

    // 2. rebuild canonical codes
    v3d_assign_canonical_codes(table);

    // 3. build decoding tree
    v3d_huff_dnode* tree = build_tree(table);

    // 4. read RLE stream size
    int size = v3d_read_bits(br, 16);

    int pos = 0;

    // 5. decode stream
    for (int i = 0; i < size; i++)
    {
        v3d_huff_dnode* cur = tree;

        // traverse tree until leaf
        while (cur->left || cur->right)
        {
            int bit = v3d_read_bits(br, 1);

            if (bit == 0)
                cur = cur->left;
            else
                cur = cur->right;

            if (!cur)
            {
                printf("Decode error: invalid tree traversal\n");
                v3d_free_decoder_tree(tree);
                return 0;
            }
        }

        int symbol = cur->symbol;

        // read run-length
        int count = v3d_read_bits(br, 8);

        for (int j = 0; j < count; j++)
        {
            output[pos++] = symbol;
        }
    }

    *out_len = pos;

    // 6. cleanup
    v3d_free_decoder_tree(tree);

    return 1;
}
