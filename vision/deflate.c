#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "deflate.h"
#include "../core/bitstream.h"
#include "../core/huffman.h"

// ==============================
// DEFLATE STATIC TABLES
// ==============================

static const int length_base[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
static const int length_extra[29] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
static const int dist_base[30]   = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const int dist_extra[30]  = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static const int code_length_order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};

// ==============================
// INTERNAL DECODER STRUCTURES
// ==============================

typedef struct {
    int left, right, symbol;
} fast_node;

/**
 * Assigns canonical Huffman codes based on bit lengths.
 */
static void assign_canonical_codes(v3d_huff_code* table, int size) {
    int bl_count[17] = {0};
    int next_code[17] = {0};

    for (int i = 0; i < size; i++) {
        if (table[i].length > 0) bl_count[table[i].length]++;
    }

    int code = 0;
    for (int bits = 1; bits <= 15; bits++) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    for (int i = 0; i < size; i++) {
        if (table[i].length > 0) {
            table[i].code = next_code[table[i].length]++;
        }
    }
}

/**
 * Builds a binary tree for fast Huffman decoding.
 */
static int build_fast_tree(fast_node* nodes, int* next_node, v3d_huff_code* table, int size) {
    int root = (*next_node)++;
    nodes[root].left = nodes[root].right = -1;
    nodes[root].symbol = -1;

    for (int i = 0; i < size; i++) {
        if (table[i].length == 0) continue;

        int cur = root;
        for (int b = 0; b < table[i].length; b++) {
            // Canonical codes are MSB-first
            int bit = (table[i].code >> (table[i].length - 1 - b)) & 1;
            int* next = (bit == 0) ? &nodes[cur].left : &nodes[cur].right;

            if (*next == -1) {
                *next = (*next_node)++;
                nodes[*next].left = nodes[*next].right = nodes[*next].symbol = -1;
            }
            cur = *next;
        }
        nodes[cur].symbol = i;
    }
    return root;
}

static int fast_decode(v3d_bitreader* br, fast_node* nodes, int root) {
    int cur = root;
    while (nodes[cur].left != -1 || nodes[cur].right != -1) {
        int bit = v3d_read_bits(br, 1);
        cur = (bit == 0) ? nodes[cur].left : nodes[cur].right;
        if (cur < 0) return -1;
    }
    return nodes[cur].symbol;
}

// ==============================
// PUBLIC INFLATE API
// ==============================

int v3d_inflate(const uint8_t* input, uint32_t input_size, v3d_raw_buffer* out) {
    if (input_size < 2) return 0;

    // Detect ZLIB header (0x78)
    int offset = (input[0] == 0x78) ? 2 : 0;
    
    v3d_bitreader br;
    v3d_bitreader_init(&br, input + offset, input_size - offset, V3D_BIT_LSB_FIRST);

    size_t out_cap = 16 * 1024 * 1024; // Initial 16MB
    uint8_t* output = (uint8_t*)malloc(out_cap);
    fast_node* nodes = (fast_node*)malloc(sizeof(fast_node) * 16384);

    if (!output || !nodes) return 0;

    int pos = 0;
    int done = 0;

    while (!done) {
        int bfinal = v3d_read_bits(&br, 1);
        int btype = v3d_read_bits(&br, 2);

        if (btype == 0) { // NO COMPRESSION
            br.bit_pos = 0; // Align to byte
            uint16_t len = (uint16_t)v3d_read_bits(&br, 16);
            uint16_t nlen = (uint16_t)v3d_read_bits(&br, 16);
            
            if (pos + len > (int)out_cap) {
                out_cap = pos + len + (1024 * 1024);
                output = (uint8_t*)realloc(output, out_cap);
            }
            
            for (int i = 0; i < len; i++) {
                output[pos++] = (uint8_t)v3d_read_bits(&br, 8);
            }
        } 
        else if (btype == 2) { // DYNAMIC HUFFMAN
            int HLIT = v3d_read_bits(&br, 5) + 257;
            int HDIST = v3d_read_bits(&br, 5) + 1;
            int HCLEN = v3d_read_bits(&br, 4) + 4;

            v3d_huff_code clt[19] = {0};
            for (int i = 0; i < HCLEN; i++) 
                clt[code_length_order[i]].length = v3d_read_bits(&br, 3);

            assign_canonical_codes(clt, 19);
            int p_ptr = 0;
            int cl_root = build_fast_tree(nodes, &p_ptr, clt, 19);

            uint8_t lens[320] = {0};
            int i = 0;
            while (i < HLIT + HDIST) {
                int s = fast_decode(&br, nodes, cl_root);
                if (s < 0) break;
                if (s < 16) lens[i++] = (uint8_t)s;
                else if (s == 16) {
                    int r = v3d_read_bits(&br, 2) + 3;
                    uint8_t prev = (i > 0) ? lens[i - 1] : 0;
                    while (r-- && i < HLIT + HDIST) lens[i++] = prev;
                } else if (s == 17) {
                    int r = v3d_read_bits(&br, 3) + 3;
                    while (r-- && i < HLIT + HDIST) lens[i++] = 0;
                } else if (s == 18) {
                    int r = v3d_read_bits(&br, 7) + 11;
                    while (r-- && i < HLIT + HDIST) lens[i++] = 0;
                }
            }

            v3d_huff_code lt[288] = {0}, dt[32] = {0};
            for (int j = 0; j < HLIT; j++) lt[j].length = lens[j];
            for (int j = 0; j < HDIST; j++) dt[j].length = lens[HLIT + j];

            assign_canonical_codes(lt, 288);
            assign_canonical_codes(dt, 32);

            p_ptr = 0;
            int lit_root = build_fast_tree(nodes, &p_ptr, lt, 288);
            int dist_root = build_fast_tree(nodes, &p_ptr, dt, 32);

            while (1) {
                int sym = fast_decode(&br, nodes, lit_root);
                if (sym == 256 || sym < 0) break;

                if (sym < 256) {
                    if (pos >= (int)out_cap) {
                        out_cap *= 2;
                        output = (uint8_t*)realloc(output, out_cap);
                    }
                    output[pos++] = (uint8_t)sym;
                } else {
                    int idx = sym - 257;
                    int len = length_base[idx] + (length_extra[idx] ? v3d_read_bits(&br, length_extra[idx]) : 0);
                    int ds = fast_decode(&br, nodes, dist_root);
                    if (ds < 0) break;
                    int dist = dist_base[ds] + (dist_extra[ds] ? v3d_read_bits(&br, dist_extra[ds]) : 0);

                    // Expand buffer if back-copying exceeds current capacity
                    if (pos + len > (int)out_cap) {
                        out_cap = (pos + len) * 2;
                        output = (uint8_t*)realloc(output, out_cap);
                    }

                    for (int k = 0; k < len; k++) {
                        output[pos] = output[pos - dist];
                        pos++;
                    }
                }
            }
        } 
        else {
            // Block type 1 (Fixed) or undefined
            done = 1;
        }

        if (bfinal) done = 1;
    }

    out->data = output;
    out->size = pos;
    free(nodes);
    return 1;
}

void v3d_free_raw(v3d_raw_buffer* buf) {
    if (buf && buf->data) {
        free(buf->data);
        buf->data = NULL;
        buf->size = 0;
    }
}
