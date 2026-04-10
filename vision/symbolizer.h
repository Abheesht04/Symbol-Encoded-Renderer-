#ifndef V3D_SYMBOLIZER_H
#define V3D_SYMBOLIZER_H

#include <stdint.h>
#include "..\\vision\\geometry.h"

void v3d_symbolize(
    uint8_t* pixels,
    int width,
    int height,
    v3d_geom_symbol* out);

#endif
