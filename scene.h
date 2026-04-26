#pragma once

#include "external/geometry.h"
#include "Player.h"
#include <stdint.h>
#include <vector>


class scene
{
public:

    Player player;

    // ===== WORLD SIZE =====
    static const int SIZE_X = 32;
    static const int SIZE_Y = 16;
    static const int SIZE_Z = 32;

    uint8_t voxels[SIZE_X][SIZE_Y][SIZE_Z];

    bool debug = false;

    void Init();
    void Update();

    int BuildGeometry(v3d_vertex* out);

    void GetMVP(float* outMVP);

    std::vector<v3d_chunk> chunks;
};