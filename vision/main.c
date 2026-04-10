#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Core V3D
#include "../core/rle.h"
#include "../core/huffman.h"
#include "../core/decode_huffman.h"
#include "../core/encoder.h"
#include "../core/decoder.h"
#include "../core/bitstream.h"

// Vision
#include "png_reader.h"
#include "deflate.h"
#include "png_process.h"

// Pipeline
#include "symbolizer.h"
#include "geometry.h"

/* =========================
    MAIN
   ========================= */

int main()
{
    printf("=== V3D FULL PIPELINE TEST ===\n\n");

    /* =========================
        1. LOAD PNG
       ========================= */

    v3d_png_info info;
    v3d_buffer idat;

    if (!v3d_png_load("test.png", &info, &idat)) {
        printf("❌ PNG load failed\n");
        return -1;
    }

    printf("✅ PNG: %ux%u | BitDepth: %u\n",
           info.width, info.height, info.bit_depth);

    /* =========================
        2. DEFLATE
       ========================= */

    v3d_raw_buffer raw;

    if (!v3d_inflate(idat.data, idat.size, &raw)) {
        printf("❌ Inflate failed\n");
        v3d_png_free(&idat);
        return -1;
    }

    // Since the output was 31MB for a 3270x2381 image, we know it's 4 BPP (RGBA)
    int bpp = 4; 
    uint32_t expected_raw = (info.width * bpp + 1) * info.height;

    printf("✅ Inflate OK | RAW size: %u bytes\n", raw.size);
    printf("🔍 Expected RAW for %d BPP: %u bytes\n\n", bpp, expected_raw);

    v3d_png_free(&idat);

    /* =========================
        3. PNG UNFILTER
       ========================= */

    printf("🔧 Running PNG unfilter...\n");
    v3d_png_unfilter(raw.data, info.width, info.height, bpp);
    printf("✅ Unfilter complete\n\n");

    /* =========================
        4. EXTRACT PIXELS (CONVERT TO HEIGHTMAP)
       ========================= */

    uint8_t* pixels = (uint8_t*)malloc(info.width * info.height);

    if (!pixels) {
        printf("❌ Pixel alloc failed\n");
        return -1;
    }

    // We extract only the Red channel (first byte of RGBA) to use as height data
    for (int y = 0; y < info.height; y++) {
        uint8_t* row_start = raw.data + y * (info.width * bpp + 1) + 1;
        for (int x = 0; x < info.width; x++) {
            pixels[y * info.width + x] = row_start[x * bpp];
        }
    }

    printf("✅ Pixels extracted (Red channel used for height)\n");

    // Debug: sample first few pixels
    printf("🔍 Pixel sample: ");
    for (int i = 0; i < 10 && i < info.width * info.height; i++)
        printf("%d ", pixels[i]);
    printf("\n\n");

    v3d_free_raw(&raw);

    /* =========================
        5. SYMBOLIZER
       ========================= */

    v3d_geom_symbol* symbols =
        (v3d_geom_symbol*)malloc(sizeof(v3d_geom_symbol) * info.width * info.height);

    if (!symbols) {
        printf("❌ Symbol alloc failed\n");
        free(pixels);
        return -1;
    }

    printf("🔧 Running symbolizer...\n");
    v3d_symbolize(pixels, info.width, info.height, symbols);
    printf("✅ Symbolization complete\n");

    free(pixels);

    // Debug: count symbols
    int edge=0, line=0, surface=0;
    for (int i = 0; i < (int)(info.width * info.height); i++) {
        if (symbols[i] == V3D_EDGE) edge++;
        else if (symbols[i] == V3D_LINE) line++;
        else if (symbols[i] == V3D_SURFACE) surface++;
    }

    printf("🔍 Symbol stats:\n");
    printf("   EDGE   : %d\n", edge);
    printf("   LINE   : %d\n", line);
    printf("   SURFACE: %d\n\n", surface);

    /* =========================
        6. GEOMETRY BUILD
       ========================= */
    
    // Estimate vertex buffer size (conservative 6 verts per symbol if using tris)
    int estimate_vertices = (edge + line + surface) * 6;

    v3d_vertex* vertices =
        (v3d_vertex*)malloc(sizeof(v3d_vertex) * estimate_vertices);

    if (!vertices) {
        printf("❌ Vertex alloc failed\n");
        free(symbols);
        return -1;
    }

    printf("🔧 Building geometry...\n");

    int vertex_count =
        v3d_build_geometry(symbols, info.width, info.height, vertices);

    printf("✅ Geometry built | Vertices: %d\n", vertex_count);

    if (vertex_count == 0) {
        printf("⚠️ WARNING: No geometry generated (check symbolizer thresholds)\n");
    }

    // Debug: print first few vertices
    printf("🔍 Vertex sample:\n");
    for (int i = 0; i < 5 && i < vertex_count; i++) {
        printf("   (%.3f, %.3f)\n", vertices[i].x, vertices[i].y);
    }
    printf("\n");

    /* =========================
        7. V3D COMPRESSION TEST
       ========================= */

    // Testing compression on a subset of the symbol stream
    int max_symbols = 100000;
    int input_size = ( (int)(info.width * info.height) > max_symbols) ? max_symbols : (info.width * info.height);

    v3d_symbol* input = (v3d_symbol*)malloc(sizeof(v3d_symbol) * input_size);

    for (int i = 0; i < input_size; i++)
        input[i] = (v3d_symbol)(symbols[i] & 0xFF);

    v3d_rle_stream rle;
    v3d_rle_encode(input, input_size, &rle);

    printf("🔍 RLE: %d → %d runs\n", input_size, rle.size);

    v3d_huff_code table[256];
    v3d_build_huffman(input, input_size, table);

    v3d_bitwriter bw;
    v3d_bitwriter_init(&bw, input_size * 2);

    v3d_full_encode(&rle, &bw, table);
    v3d_bitwriter_flush(&bw);

    printf("✅ V3D Encoded size: %llu bytes\n",
           (unsigned long long)bw.byte_pos);

    /* =========================
        8. CLEANUP
       ========================= */

    v3d_rle_free(&rle);
    v3d_bitwriter_free(&bw);

    free(input);
    free(symbols);
    free(vertices);

    printf("\n=== PIPELINE COMPLETE ===\n");

    return 0;
}
