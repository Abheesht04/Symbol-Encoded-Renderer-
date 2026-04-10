#ifdef IMG_H
#define IMG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct{
	uint16_t

}pixel;

typedef struct{
	usigned int rows;
	unsigned int cols;
	unsigned int maxval;

	pixel**pixels;
} ppmimage;



#endif

