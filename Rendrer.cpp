
#include "Rendrer.h"
#include "DevideManager.h" 
#include "D:\smallEngine\external\geometry.h"
#include <d3dcompiler.h>
#include <vector>
#include <math.h>
#include <comdef.h> // For _com_error

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace Microsoft::WRL;

const float BLOCK_SPACING = 0.65f;
const float CHUNK_WORLD_SIZE = (float)CHUNK_SIZE * BLOCK_SPACING;


struct v3d_instance_fixed {
    float x, y, z;       // 12 bytes
    float symbol;        // 4 bytes  (Total 16)
    float nx, ny, nz;    // 12 bytes (Total 28)
    float padding;       // 4 bytes  (Total 32)
    float r, g, b, a;    // 16 bytes (Total 48)
};

#define DX_CHECK(x)                                         \
{                                                           \
    HRESULT hr__ = (x);                                     \
    if (FAILED(hr__)) {                                     \
        _com_error err(hr__);                               \
        OutputDebugStringW(L"DX_CHECK FAILURE: ");          \
        OutputDebugStringW(err.ErrorMessage());             \
        OutputDebugStringW(L"\n");                          \
        __debugbreak();                                     \
    }                                                       \
}
void Rendrer::Init(DeviceManager* manager) {
    auto device = manager->GetDevice();

    // 1. Initialize Scene/Player logic
    sc.Init();

    // 2. Root Signature
    D3D12_DESCRIPTOR_RANGE range = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND };
    D3D12_ROOT_PARAMETER rootParams[2] = {};

    // Matrix Buffer (Slot 0)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Texture Table (Slot 1)
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &range;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // Crispy voxel look
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = { 2, rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };

    ComPtr<ID3DBlob> sig, err;
    if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err))) {
        if (err) OutputDebugStringA((const char*)err->GetBufferPointer());
        return;
    }
    DX_CHECK(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

    // 3. PSO Setup (Shaders)
    ComPtr<ID3DBlob> vs, ps, gs, sErr;
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    if (FAILED(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vs, &sErr))) {
        if (sErr) { OutputDebugStringA((const char*)sErr->GetBufferPointer()); __debugbreak(); }
    }
    DX_CHECK(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "GSMain", "gs_5_0", compileFlags, 0, &gs, nullptr));
    DX_CHECK(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &ps, nullptr));

    D3D12_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT,           0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // symbol (padding1)
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // nx, ny, nz
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }  // r, g, b, a
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.InputLayout = { layout, _countof(layout) };
    pso.pRootSignature = rootSignature.Get();
    pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    pso.GS = { gs->GetBufferPointer(), gs->GetBufferSize() };
    pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };

    // Rasterizer: Use standard Back-face culling
    pso.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState.DepthClipEnable = TRUE;

    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    pso.SampleMask = UINT_MAX;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pso.SampleDesc.Count = 1;

    // Depth: Use LESS to prevent flickering (Z-fighting)
    pso.DepthStencilState.DepthEnable = TRUE;
    pso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    DX_CHECK(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState)));

    // 4. Heaps & Dummy Texture
    D3D12_DESCRIPTOR_HEAP_DESC srvH = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
    DX_CHECK(device->CreateDescriptorHeap(&srvH, IID_PPV_ARGS(&srvHeap)));

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.Width = 1; texDesc.Height = 1; texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1; texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_HEAP_PROPERTIES defP = { D3D12_HEAP_TYPE_DEFAULT };
    DX_CHECK(device->CreateCommittedResource(&defP, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&m_dummyTexture)));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_dummyTexture.Get(), &srvDesc, srvHeap->GetCPUDescriptorHandleForHeapStart());

    // 5. Matrix / Constant Buffer
    D3D12_HEAP_PROPERTIES up = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC cbD = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, 256, 1, 1, 1, DXGI_FORMAT_UNKNOWN, {1,0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    DX_CHECK(device->CreateCommittedResource(&up, D3D12_HEAP_FLAG_NONE, &cbD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer)));
    constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&cbvDataBegin));

    // 6. Depth Buffer (Ensures 3D objects don't overlap randomly)
    D3D12_RESOURCE_DESC dD = {};
    dD.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dD.Width = 1280; dD.Height = 720; dD.DepthOrArraySize = 1; dD.MipLevels = 1;
    dD.Format = DXGI_FORMAT_D32_FLOAT;
    dD.SampleDesc.Count = 1;
    dD.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dD.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE dC = { DXGI_FORMAT_D32_FLOAT };
    dC.DepthStencil.Depth = 1.0f;

    DX_CHECK(device->CreateCommittedResource(&defP, D3D12_HEAP_FLAG_NONE, &dD, D3D12_RESOURCE_STATE_DEPTH_WRITE, &dC, IID_PPV_ARGS(&depthBuffer)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvH = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1 };
    DX_CHECK(device->CreateDescriptorHeap(&dsvH, IID_PPV_ARGS(&dsvHeap)));
    device->CreateDepthStencilView(depthBuffer.Get(), nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // 7. Initialize Chunk Slots
    for (int i = 0; i < 16; i++) {
        m_chunks[i].state = CHUNK_STATE_EMPTY;
        m_chunks[i].x = m_chunks[i].z = -999;
    }

    // 8. Viewport & Scissor (Matches 720p window)
    viewport = { 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f };
    scissorRect = { 0, 0, 1280, 720 };
}

void Rendrer::update(DeviceManager* manager) {
    // 1. Safety Check: Verify the compiler sees exactly 48 bytes
    static_assert(sizeof(v3d_instance_fixed) == 48, "v3d_instance_fixed must be exactly 48 bytes!");

    sc.Update(); // Update camera/player movement
    auto device = manager->GetDevice();

    int currentCX = (int)floor(sc.player.x / CHUNK_WORLD_SIZE);
    int currentCZ = (int)floor(sc.player.z / CHUNK_WORLD_SIZE);

    for (int slot = 0; slot < 16; slot++) {
        int tx = currentCX + (slot % 4) - 2;
        int tz = currentCZ + (slot / 4) - 2;
        v3d_chunk* chunk = &m_chunks[slot];

        // 2. Check if chunk needs rebuilding
        if (chunk->x != tx || chunk->z != tz || chunk->state == CHUNK_STATE_EMPTY) {

            // Prevent GPU from reading buffer while we rewrite it
            manager->FlushGPU();

            std::vector<v3d_instance_fixed> instances;
            instances.reserve(CHUNK_SIZE * CHUNK_SIZE); // Performance optimization

            for (int lz = 0; lz < CHUNK_SIZE; lz++) {
                for (int lx = 0; lx < CHUNK_SIZE; lx++) {
                    float wx = (float)(tx * CHUNK_SIZE + lx);
                    float wz = (float)(tz * CHUNK_SIZE + lz);

                    // Procedural terrain height
                    float y = sinf(wx * 0.15f) * cosf(wz * 0.15f) * 2.0f;

                    v3d_instance_fixed inst = {};
                    inst.x = wx * BLOCK_SPACING;
                    inst.y = y;
                    inst.z = wz * BLOCK_SPACING;
                    inst.symbol = (y > 0.5f) ? 2.0f : 1.0f;

                    // Fill Normals (Default to Up)
                    inst.nx = 0.0f; inst.ny = 1.0f; inst.nz = 0.0f;
                    inst.padding = 0.0f;

                    // Assign Colors based on height
                    if (inst.symbol > 1.5f) { // Grass
                        inst.r = 0.2f; inst.g = 0.7f; inst.b = 0.2f; inst.a = 1.0f;
                    }
                    else { // Stone
                        inst.r = 0.5f; inst.g = 0.5f; inst.b = 0.5f; inst.a = 1.0f;
                    }

                    instances.push_back(inst);
                }
            }

            // Fix C26451: Cast to size_t/UINT64 before multiplication
            UINT64 bufSize = (UINT64)instances.size() * (UINT64)sizeof(v3d_instance_fixed);

            // 3. Create or Reuse Buffer
            if (m_chunkBuffers[slot] == nullptr || m_chunkViews[slot].SizeInBytes < bufSize) {
                m_chunkBuffers[slot].Reset();

                D3D12_HEAP_PROPERTIES upH = { D3D12_HEAP_TYPE_UPLOAD };
                D3D12_RESOURCE_DESC vD = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, bufSize, 1, 1, 1,
                                           DXGI_FORMAT_UNKNOWN, {1,0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

                DX_CHECK(device->CreateCommittedResource(&upH, D3D12_HEAP_FLAG_NONE, &vD,
                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_chunkBuffers[slot])));
            }

            // 4. Map and Copy data (Fix C4244 by casting bufSize to size_t)
            void* pD = nullptr;
            DX_CHECK(m_chunkBuffers[slot]->Map(0, nullptr, &pD));
            memcpy(pD, instances.data(), (size_t)bufSize);
            m_chunkBuffers[slot]->Unmap(0, nullptr);

            // 5. Update View for Draw call
            m_chunkViews[slot].BufferLocation = m_chunkBuffers[slot]->GetGPUVirtualAddress();
            m_chunkViews[slot].SizeInBytes = (UINT)bufSize;
            m_chunkViews[slot].StrideInBytes = (UINT)sizeof(v3d_instance_fixed);
            m_chunkVertexCounts[slot] = (UINT)instances.size();

            chunk->x = tx; chunk->z = tz;
            chunk->state = CHUNK_STATE_READY;
        }
    }

    // 6. Global Uniform Update (MVP Matrix)
    float rawMvp[16];
    sc.GetMVP(rawMvp);
    if (cbvDataBegin) {
        memcpy(cbvDataBegin, rawMvp, sizeof(float) * 16);
    }
}

void Rendrer::render(DeviceManager* manager) {
    if (!pipelineState) return;

    // 1. SYNC: Get index and wait for the GPU to be done with THIS index's resources
    UINT frameIdx = manager->GetSwapChain()->GetCurrentBackBufferIndex();
    manager->WaitForFrame();

    auto allocator = manager->GetCommandAllocator(frameIdx);
    auto cl = manager->GetCommandList();
    auto queue = manager->GetCommandQueue();

    // 2. RESET: Now safe because WaitForFrame() finished
    DX_CHECK(allocator->Reset());
    DX_CHECK(cl->Reset(allocator, pipelineState.Get()));

    // 3. BARRIER: Transition Back Buffer to Render Target
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = manager->GetRenderTarget(frameIdx);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    cl->ResourceBarrier(1, &barrier);

    // 4. SETUP: Viewports and Targets
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = manager->GetRtvHeap()->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += frameIdx * manager->GetRtvDescriptorSize();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();

    float skyColor[] = { 0.45f, 0.65f, 0.95f, 1.0f };
    cl->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    cl->ClearRenderTargetView(rtv, skyColor, 0, nullptr);
    cl->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    cl->SetGraphicsRootSignature(rootSignature.Get());
    ID3D12DescriptorHeap* heaps[] = { srvHeap.Get() };
    cl->SetDescriptorHeaps(1, heaps);

    // Bind MVP and Texture
    cl->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress());
    cl->SetGraphicsRootDescriptorTable(1, srvHeap->GetGPUDescriptorHandleForHeapStart());

    cl->RSSetViewports(1, &viewport);
    cl->RSSetScissorRects(1, &scissorRect);
    cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

    // 5. DRAW: Only draw ready chunks
    for (int i = 0; i < 16; i++) {
        if (m_chunks[i].state == CHUNK_STATE_READY && m_chunkBuffers[i]) {
            cl->IASetVertexBuffers(0, 1, &m_chunkViews[i]);
            cl->DrawInstanced(m_chunkVertexCounts[i], 1, 0, 0);
        }
    }

    // 6. PRESENT: Transition back and Execute
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    cl->ResourceBarrier(1, &barrier);

    DX_CHECK(cl->Close());
    ID3D12CommandList* lists[] = { cl };
    queue->ExecuteCommandLists(1, lists);

    manager->Signal(); // Mark this frame's fence
    DX_CHECK(manager->GetSwapChain()->Present(1, 0));
    manager->MoveToNextFrame(); // Advance to next frame and wait if necessary
}