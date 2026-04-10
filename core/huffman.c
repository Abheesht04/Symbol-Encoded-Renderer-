#include "huffman.h"
#include <stdlib.h>
#include <string.h>

/* Create node */
static v3d_huff_node* create_node(v3d_symbol sym, int freq)
{
    v3d_huff_node* n = (v3d_huff_node*)malloc(sizeof(v3d_huff_node));
    n->symbol = sym;
    n->freq = freq;
    n->left = n->right = NULL;
    return n;
}

/* Find 2 smallest nodes */
static void find_two_smallest(v3d_huff_node** nodes, int count, int* i1, int* i2)
{
    *i1 = -1; *i2 = -1;

    for (int i = 0; i < count; i++)
    {
        if (!nodes[i]) continue;

        if (*i1 == -1 || nodes[i]->freq < nodes[*i1]->freq)
        {
            *i2 = *i1;
            *i1 = i;
        }
        else if (*i2 == -1 || nodes[i]->freq < nodes[*i2]->freq)
        {
            *i2 = i;
        }
    }
}

/* Build codes recursively */
static void build_codes(v3d_huff_node* node,
                        v3d_huff_code table[256],
                        uint32_t code,
                        uint8_t length)
{
    if (!node->left && !node->right)
    {
        table[node->symbol].code = code;
        table[node->symbol].length = length;
        return;
    }

    if (node->left)
        build_codes(node->left, table, code, length + 1);

    if (node->right)
        build_codes(node->right, table,
                    code | (1 << length),
                    length + 1);
}

int v3d_build_huffman(
    const v3d_symbol* data,
    int length,
    v3d_huff_code table[256])
{
    int freq[256] = {0};

    // count frequency
    for (int i = 0; i < length; i++)
        freq[data[i]]++;

    // create initial nodes
    v3d_huff_node* nodes[256] = {0};
    int count = 0;

    for (int i = 0; i < 256; i++)
    {
        if (freq[i] > 0)
            nodes[count++] = create_node(i, freq[i]);
    }

    // build tree
    while (count > 1)
    {
        int i1, i2;
        find_two_smallest(nodes, count, &i1, &i2);

        v3d_huff_node* merged =
            create_node(0,
                nodes[i1]->freq + nodes[i2]->freq);

        merged->left = nodes[i1];
        merged->right = nodes[i2];

        nodes[i1] = merged;
        nodes[i2] = nodes[count - 1];
        count--;
    }

    memset(table, 0, sizeof(v3d_huff_code) * 256);

    if (count == 1)
        build_codes(nodes[0], table, 0, 0);

    return 1;
}


