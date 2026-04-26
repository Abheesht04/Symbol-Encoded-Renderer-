#pragma once
#include <vector>
#include <string>
#include "D:\smallEngine\external\geometry.h"

class SceneLoader {
public:
    // Loads a compressed .v3d file using Huffman-based decoding
    static std::vector<v3d_instance> LoadV3DScene(const std::string& filePath);

private:
    // Your internal Huffman bit-reading logic
    struct BitReader {
        const unsigned char* data;
        size_t bitPos;
        uint32_t ReadBits(int count);
    };
};