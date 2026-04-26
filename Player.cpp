#include "Player.h"
#include <windows.h>
#include <math.h>
#include <DirectXMath.h>

void Player::HandleMouse(float dx, float dy) {
    yaw += dx * sensitivity;
    pitch -= dy * sensitivity;

    // Constrain pitch (slightly less than 90 degrees to avoid gimbal lock)
    if (pitch > 1.55f) pitch = 1.55f;
    if (pitch < -1.55f) pitch = -1.55f;
}

void Player::GetViewMatrix(float* m) {
    using namespace DirectX;

    XMVECTOR pos = XMVectorSet(x, y, z, 0.0f);

    // Precise Direction calculation
    float cosP = cosf(pitch);
    XMVECTOR dir = XMVectorSet(
        sinf(yaw) * cosP,
        sinf(pitch),
        cosf(yaw) * cosP,
        0.0f
    );

    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX viewMat = XMMatrixLookToLH(pos, dir, up);

    XMStoreFloat4x4((XMFLOAT4X4*)m, viewMat);
}

void Player::Update() {
    float speedMult = speed;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) speedMult *= 2.0f;

    // Forward vector (XZ plane only for standard walking, or 3D for flying)
    float fx = sinf(yaw);
    float fz = cosf(yaw);

    // Right vector (perpendicular to forward)
    float rx = cosf(yaw);
    float rz = -sinf(yaw);

    if (GetAsyncKeyState('W') & 0x8000) { x += fx * speedMult; z += fz * speedMult; }
    if (GetAsyncKeyState('S') & 0x8000) { x -= fx * speedMult; z -= fz * speedMult; }
    if (GetAsyncKeyState('A') & 0x8000) { x -= rx * speedMult; z -= rz * speedMult; }
    if (GetAsyncKeyState('D') & 0x8000) { x += rx * speedMult; z += rz * speedMult; }

    if (GetAsyncKeyState(VK_SPACE) & 0x8000) y += speedMult;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) y -= speedMult;
}

void Player::GetMVP(float* mvp) {
    using namespace DirectX;

    // Projection
    float fov = 75.0f * (XM_PI / 180.0f);
    float aspect = 1280.0f / 720.0f;
    XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, aspect, 0.1f, 1000.0f);

    // View
    XMVECTOR pos = XMVectorSet(x, y, z, 0.0f);
    float cosP = cosf(pitch);
    XMVECTOR dir = XMVectorSet(sinf(yaw) * cosP, sinf(pitch), cosf(yaw) * cosP, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookToLH(pos, dir, up);

    // Combined
    XMMATRIX combined = XMMatrixMultiply(view, proj);
    XMStoreFloat4x4((XMFLOAT4X4*)mvp, combined);
}