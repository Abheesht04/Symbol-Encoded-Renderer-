#ifndef V3D_DEFLATE_H
#define V3D_DEFLATE_H

#include "D:\machine_vision_1\core\bitstream.h" 
#include <stdint.h>

typedef struct{
	uint8_t*data;
	uint32_t size;

}v3d_raw_buffer;

int v3d_inflate(const uint8_t*input,uint32_t input_size,v3d_raw_buffer*out);

void v3d_free_raw(v3d_raw_buffer*buf);

#endif

