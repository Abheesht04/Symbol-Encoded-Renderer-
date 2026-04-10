#ifndef V3D_PNG_H
#define V3D_PNG_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t bit_depth;
    uint8_t color_type;
} v3d_png_info;

typedef struct {
    uint8_t* data;
    uint32_t size;
} v3d_buffer;

int v3d_png_check_signature(FILE* f);

/* load PNG → extract metadata + IDAT */
int v3d_png_load(
    const char* filename,
    v3d_png_info* info,
    v3d_buffer* idat);

/* free buffer */
void v3d_png_free(v3d_buffer* buf);

#endif
