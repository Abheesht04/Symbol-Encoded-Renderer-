#pragma once
#include "DevideManager.h"
#include "scene.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "D:/smallEngine/external/geometry.h"


// Matches your VS_INPUT in HLSL
struct Vertx {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;
};

class Rendrer
{
public:
    
    void Init(DeviceManager* manager);
    void update(DeviceManager* manager);
    void render(DeviceManager* manager);
    scene sc;
    
private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

    // Resource Management
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

    // Synchronization
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue;
    HANDLE fenceEvent;

    // Display
    D3D12_VIEWPORT viewport;
    D3D12_RECT scissorRect;

    int vertexCount = 0;

    ComPtr<ID3D12Resource> constantBuffer;
    UINT8* cbvDataBegin = nullptr;


    ComPtr<ID3D12Resource> depthBuffer;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
     
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12Resource> textureResource;


    v3d_chunk m_chunks[16];

    ComPtr<ID3D12Resource> m_chunkBuffers[16];

    D3D12_VERTEX_BUFFER_VIEW m_chunkViews[16];

    UINT m_chunkVertexCounts[16];
    
    ComPtr<ID3D12Resource> m_dummyTexture;

};