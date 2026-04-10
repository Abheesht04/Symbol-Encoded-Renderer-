#include "png_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================
   Read big-endian 32-bit
   ================================ */
static uint32_t read_u32(FILE* f)
{
    uint8_t b[4];
    fread(b, 1, 4, f);

    return (b[0]<<24) | (b[1]<<16) | (b[2]<<8) | b[3];
}

/* ================================
   PNG SIGNATURE CHECK
   ================================ */
int v3d_png_check_signature(FILE* f)
{
    uint8_t header[8];
    uint8_t signature[8] = {
        137,80,78,71,13,10,26,10
    };

    if (fread(header, 1, 8, f) != 8)
        return 0;

    return memcmp(header, signature, 8) == 0;
}

/* ================================
   LOAD PNG
   ================================ */
int v3d_png_load(
    const char* filename,
    v3d_png_info* info,
    v3d_buffer* idat)
{
    FILE* f = fopen(filename, "rb");

    if (!f)
        return 0;

    /* --- Signature --- */
    if (!v3d_png_check_signature(f))
    {
        fclose(f);
        return 0;
    }

    idat->data = NULL;
    idat->size = 0;

    while (1)
    {
        uint32_t length = read_u32(f);

        char type[5] = {0};
        fread(type, 1, 4, f);

        uint8_t* chunk_data = (uint8_t*)malloc(length);
        fread(chunk_data, 1, length, f);

        uint32_t crc = read_u32(f); // ignored

        /* ---- IHDR ---- */
        if (strcmp(type, "IHDR") == 0)
        {
            info->width =
                (chunk_data[0]<<24) |
                (chunk_data[1]<<16) |
                (chunk_data[2]<<8)  |
                chunk_data[3];

            info->height =
                (chunk_data[4]<<24) |
                (chunk_data[5]<<16) |
                (chunk_data[6]<<8)  |
                chunk_data[7];

            info->bit_depth = chunk_data[8];
            info->color_type = chunk_data[9];
        }

        /* ---- IDAT ---- */
        else if (strcmp(type, "IDAT") == 0)
        {
            uint8_t* new_data = (uint8_t*)realloc(
                idat->data,
                idat->size + length);

            if (!new_data)
            {
                free(chunk_data);
                fclose(f);
                return 0;
            }

            idat->data = new_data;

            memcpy(idat->data + idat->size,
                   chunk_data,
                   length);

            idat->size += length;
        }

        /* ---- IEND ---- */
        else if (strcmp(type, "IEND") == 0)
        {
            free(chunk_data);
            break;
        }

        free(chunk_data);
    }

    fclose(f);
    return 1;
}

/* ================================
   FREE BUFFER
   ================================ */
void v3d_png_free(v3d_buffer* buf)
{
    free(buf->data);
}
