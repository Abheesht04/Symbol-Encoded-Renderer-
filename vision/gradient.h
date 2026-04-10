#ifndef V3D_BITWRITER_H
#define V3D_BITWRITER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t* data;
    size_t capacity;
    size_t byte_pos;

    uint8_t current_byte;
    uint8_t bit_pos;
} v3d_bitwriter;

int v3d_bitwriter_init(v3d_bitwriter* bw, size_t capacity);

void v3d_write_bits(v3d_bitwriter* bw, uint32_t value, int count);

void v3d_bitwriter_flush(v3d_bitwriter* bw);

void v3d_bitwriter_free(v3d_bitwriter* bw);

#endif
