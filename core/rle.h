#ifndef V3D_RLE_H
#define V3D_RLE_H

#include "symbol.h"

typedef struct{
	v3d_symbol symbol;
	uint32_t count;
}v3d_rle_pair;

typedef struct{
	v3d_rle_pair*data;
	int size;

}v3d_rle_stream;

int v3d_rle_encode(const v3d_symbol*input,int length,v3d_rle_stream*out);

void v3d_rle_free(v3d_rle_stream*stream);

#endif
