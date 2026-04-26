#include "scene.h"
#include "external/geometry.h"
#include <math.h>
#include <string.h>
#include <vector>
#include <DirectXMath.h>

#define CHUNK_SIZE 16
std::vector<v3d_chunk> chunks;

// ========================================================
// GENERATOR HELPERS
// ========================================================
float GetHeightAt(float x, float z) {
    float fx = x * 0.15f;
    float fz = z * 0.15f;
    float h = sinf(fx) * 2.0f + cosf(fz) * 2.0f;
    return h + 10.0f; // Base floor level
}

// ========================================================
// INIT — Setup Chunks and Player
// ========================================================
void scene::Init()
{
    // 1. Initialize Chunks with Symbols
    // For this engine, we interpret the world as a grid of symbols

    player.x = 4.0f;
    player.z = 4.0f;
    player.y = 20.0f; // High enough to see the whole 12.8-unit world
    player.yaw = 0.0f;
    player.pitch = -0.5f; // Look slightly downward

    for (int cz = 0; cz < 2; cz++) {
        for (int cx = 0; cx < 2; cx++) {
            v3d_chunk newChunk;
            newChunk.x = cx;
            newChunk.z = cz;

            for (int z = 0; z < CHUNK_SIZE; z++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    float worldX = (float)(cx * CHUNK_SIZE + x);
                    float worldZ = (float)(cz * CHUNK_SIZE + z);
                    float h = GetHeightAt(worldX, worldZ);

                    // Determine symbol based on height/position
                    int idx = z * CHUNK_SIZE + x;
                    if (h > 12.5f) {
                        newChunk.nodes[idx] = V3D_CORNER; // Tall points become pillars
                    }
                    else {
                        newChunk.nodes[idx] = V3D_SURFACE; // Rest is flat surface
                    }
                }
            }
            chunks.push_back(newChunk);
        }
    }

    // 2. Spawn Player above the first chunk
    player.x = (CHUNK_SIZE * 0.5f) * 0.4f;
    player.z = (CHUNK_SIZE * 0.5f) * 0.4f;
    player.y = GetHeightAt(player.x / 0.4f, player.z / 0.4f) + 2.5f;
}

void scene::Update()
{
    player.Update();
}

// ========================================================
// GEOMETRY GENERATOR — Node-to-Shape Logic
// ========================================================
void AddQuad(v3d_vertex* out, int& count, float x, float y, float z, float size, float r, float g, float b) {
    // Helper to build a 2-triangle surface quad
    // Using member-wise assignment to avoid E0146 "too many initializer values"
    v3d_vertex v[4];
    float px[4] = { x, x + size, x, x + size };
    float pz[4] = { z, z, z + size, z + size };

    for (int i = 0; i < 4; i++) {
        v[i].x = px[i]; v[i].y = y; v[i].z = pz[i]; v[i].padding1 = 1.0f;
        v[i].nx = 0.0f; v[i].ny = 1.0f; v[i].nz = 0.0f; v[i].padding2 = 0.0f;
        v[i].r = r; v[i].g = g; v[i].b = b; v[i].a = 1.0f;
    }

    // Triangle 1
    out[count++] = v[0]; out[count++] = v[2]; out[count++] = v[1];
    // Triangle 2
    out[count++] = v[1]; out[count++] = v[2]; out[count++] = v[3];
}

int scene::BuildGeometry(v3d_vertex* out)
{
    int count = 0;
    const float scale = 0.65f; // Matches your BLOCK_SPACING

    for (auto& chunk : chunks) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                v3d_geom_symbol sym = chunk.nodes[z * CHUNK_SIZE + x];
                if (sym == V3D_EMPTY) continue;

                float wx = (chunk.x * CHUNK_SIZE + x) * scale;
                float wz = (chunk.z * CHUNK_SIZE + z) * scale;
                float wy = GetHeightAt((float)(chunk.x * CHUNK_SIZE + x), (float)(chunk.z * CHUNK_SIZE + z));

                // Send exactly ONE vertex. The Geometry Shader will turn this into a 3D cube.
                v3d_vertex& v = out[count++];
                v.x = wx;
                v.y = wy;
                v.z = wz;
                v.padding1 = (float)sym; // Pass the symbol type here (V3D_SURFACE, etc.)

                // Set a default color
                v.r = (sym == V3D_CORNER) ? 0.8f : 0.4f;
                v.g = 0.7f;
                v.b = 0.3f;
                v.a = 1.0f;
            }
        }
    }
    return count;
}

// ========================================================
// MVP MATH — Perspective and View
// ========================================================
void scene::GetMVP(float* outMVP)
{
    using namespace DirectX;

    // 1. Projection
    float fov = 75.0f * (3.14159f / 180.0f);
    float aspect = 1280.0f / 720.0f;
    XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, aspect, 0.1f, 1000.0f);

    // 2. View (Camera Rotation + Position)
    XMVECTOR pos = XMVectorSet(player.x, player.y, player.z, 0.0f);

    // Calculate look direction from player yaw/pitch
    float cosP = cosf(player.pitch);
    XMVECTOR dir = XMVectorSet(
        sinf(player.yaw) * cosP,
        sinf(player.pitch),
        cosf(player.yaw) * cosP,
        0.0f
    );
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookToLH(pos, dir, up);

    // 3. Combine and Transpose for HLSL
    XMMATRIX combined = XMMatrixMultiply(view, proj);
    XMMATRIX transposed = XMMatrixTranspose(combined);

    XMStoreFloat4x4((XMFLOAT4X4*)outMVP, transposed);
}