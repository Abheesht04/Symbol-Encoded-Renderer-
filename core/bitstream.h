#ifndef V3D_BITSTREAM_H
#define V3D_BITSTREAM_H

#include <stdint.h>
#include <stddef.h>

/* =============================
   BIT ORDER
   ============================= */


typedef enum {
    V3D_BIT_LSB_FIRST = 0,
    V3D_BIT_MSB_FIRST = 1
} v3d_bit_order;

/* =============================
   BIT READER
   ============================= */

typedef struct {
    const uint8_t* data;
    size_t size;

    size_t byte_pos;
    uint8_t bit_pos;

    v3d_bit_order order;
} v3d_bitreader;

void v3d_bitreader_init(
    v3d_bitreader* br,
    const uint8_t* data,
    size_t size,
    v3d_bit_order order);

uint32_t v3d_read_bits(
    v3d_bitreader* br,
    int count);

/* =============================
   BIT WRITER
   ============================= */

typedef struct {
    uint8_t* data;
    size_t capacity;
    size_t byte_pos;

    uint8_t current_byte;
    uint8_t bit_pos;
} v3d_bitwriter;

int v3d_bitwriter_init(
    v3d_bitwriter* bw,
    size_t capacity);

void v3d_write_bits(
    v3d_bitwriter* bw,
    uint32_t value,
    int count);

void v3d_bitwriter_flush(v3d_bitwriter* bw);

void v3d_bitwriter_free(v3d_bitwriter* bw);
void v3d_bitreader_align(v3d_bitreader* br);

#endif
