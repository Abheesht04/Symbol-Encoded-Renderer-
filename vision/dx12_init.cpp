#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include "geometry.h"
#include "D:\machine_vision_1\core\decoder.h"
#include "png_reader.h"
#include "deflate.h"
#include "png_process.h"
#include "symbolizer.h"
#include "geometry.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

// --- Structures ---
struct Vertex {
    float position[3];
    float color[4];
};

// --- Pipeline Globals ---
const UINT FRAME_COUNT = 2;
UINT g_width = 1280;
UINT g_height = 720;

ComPtr<ID3D12Device> g_device;
ComPtr<ID3D12CommandQueue> g_commandQueue;
ComPtr<IDXGISwapChain3> g_swapChain;
ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
ComPtr<ID3D12Resource> g_renderTargets[FRAME_COUNT];
ComPtr<ID3D12CommandAllocator> g_commandAllocator;
ComPtr<ID3D12GraphicsCommandList> g_commandList;

// --- Pipeline State ---
ComPtr<ID3D12RootSignature> g_rootSignature;
ComPtr<ID3D12PipelineState> g_pipelineState;
ComPtr<ID3D12Resource> g_vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW g_vertexBufferView;

// --- Synchronization ---
UINT g_frameIndex;
HANDLE g_fenceEvent;
ComPtr<ID3D12Fence> g_fence;
UINT64 g_fenceValue;

ComPtr<ID3D12Resource>g_cameraBuffer;
UINT8*g_cameraPtr = nullptr;
struct CameraCB{
	float mvp[16];
};


int BuildLinesFromSymbols(
    v3d_symbol* symbols,
    int count,
    Vertex* outVertices)
{
    int v = 0;

    for (int i = 0; i < count; i++)
    {
        if (symbols[i] == V3D_LINE)
        {
            float x = (float)i / 100.0f;

            // simple vertical line
            outVertices[v++] = { { x,  0.5f, 0.0f }, {1,1,1,1} };
            outVertices[v++] = { { x, -0.5f, 0.0f }, {1,1,1,1} };
        }
    }

    return v;
}

// --- 1. Infrastructure Initialization ---
void InitD3D12(HWND hwnd) {
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    // Device
    D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_device));

    // Command Queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    g_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_commandQueue));

    // Swap Chain (Manual Desc)
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.Width = g_width;
    swapChainDesc.Height = g_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    factory->CreateSwapChainForHwnd(g_commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain);
    swapChain.As(&g_swapChain);
    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

    // RTV Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FRAME_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    g_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_rtvHeap));

    UINT rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT n = 0; n < FRAME_COUNT; n++) {
        g_swapChain->GetBuffer(n, IID_PPV_ARGS(&g_renderTargets[n]));
        g_device->CreateRenderTargetView(g_renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize;
    }

    g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator));
}

// --- 2. Shader & Pipeline State (Pure Manual) ---
void CreatePipeline() {
    // Root Signature
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = 0;
    param.Descriptor.RegisterSpace = 0;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = &param;

    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    g_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature));

    // Shaders
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vertexShader, nullptr);
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixelShader, nullptr);

    // Input Layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Rasterizer State
    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // Blend State
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRTBlendDesc = {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blendDesc.RenderTarget[i] = defaultRTBlendDesc;

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = g_rootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineState));

    // Create command list (closed)
    g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator.Get(), g_pipelineState.Get(), IID_PPV_ARGS(&g_commandList));
    g_commandList->Close();
}

// --- 3. Resource Creation (Pure Manual Heap/Desc) ---

void CreateVertexBuffer()
{
    // -------------------------
    // 1. LOAD PNG
    // -------------------------
    v3d_png_info info;
    v3d_buffer idat;

    if (!v3d_png_load("test.png", &info, &idat))
    {
        printf("PNG load failed\n");
        return;
    }

    // -------------------------
    // 2. DEFLATE
    // -------------------------
    v3d_raw_buffer raw;

    if (!v3d_inflate(idat.data, idat.size, &raw))
    {
        printf("Inflate failed\n");
        return;
    }

    // -------------------------
    // 3. UNFILTER
    // -------------------------
    int bpp = 1; // grayscale

    v3d_png_unfilter(raw.data, info.width, info.height, bpp);

    // -------------------------
    // 4. EXTRACT PIXELS
    // -------------------------
    uint8_t* pixels =
        (uint8_t*)malloc(info.width * info.height);

    for (int y = 0; y < info.height; y++)
    {
        memcpy(
            pixels + y * info.width,
            raw.data + y * (info.width + 1) + 1,
            info.width
        );
    }

    // -------------------------
    // 5. SYMBOLIZE
    // -------------------------
    v3d_geom_symbol* symbols =
        (v3d_geom_symbol*)malloc(sizeof(v3d_geom_symbol) * info.width * info.height);

    v3d_symbolize(pixels, info.width, info.height, symbols);

    // -------------------------
    // 6. BUILD GEOMETRY
    // -------------------------
        v3d_vertex* v3dVertices =
        (v3d_vertex*)malloc(sizeof(v3d_vertex) * info.width * info.height * 4);

    int vertexCount =
        v3d_build_geometry(symbols, info.width, info.height, v3dVertices);

    printf("Vertices: %d\n", vertexCount);

    // -------------------------
    // 7. CREATE GPU BUFFER
    // -------------------------
    const UINT vertexBufferSize =
        vertexCount * sizeof(Vertex);

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = vertexBufferSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    g_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&g_vertexBuffer));

    UINT8* pData;
    D3D12_RANGE range = {0,0};

    g_vertexBuffer->Map(0, &range, (void**)&pData);

    memcpy(pData, v3dVertices, vertexBufferSize);

    g_vertexBuffer->Unmap(0, nullptr);

    g_vertexBufferView.BufferLocation =
        g_vertexBuffer->GetGPUVirtualAddress();

    g_vertexBufferView.StrideInBytes = sizeof(Vertex);
    g_vertexBufferView.SizeInBytes = vertexBufferSize;



   // -------------------------
   // CAMERA BUFFER
   // -------------------------
   D3D12_HEAP_PROPERTIES heapPropsCB = {};
   heapPropsCB.Type = D3D12_HEAP_TYPE_UPLOAD;

   D3D12_RESOURCE_DESC cbDesc = {};
   cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   cbDesc.Width = 1024; // must be 256-aligned
   cbDesc.Height = 1;
   cbDesc.DepthOrArraySize = 1;
   cbDesc.MipLevels = 1;
   cbDesc.SampleDesc.Count = 1;
   cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    g_device->CreateCommittedResource(
    &heapPropsCB,
    D3D12_HEAP_FLAG_NONE,
    &cbDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&g_cameraBuffer));

    // Map once
    g_cameraBuffer->Map(0, nullptr, (void**)&g_cameraPtr); 


   


    // -------------------------
    // 8. CLEANUP
    // -------------------------
    free(pixels);
    free(symbols);
    free(v3dVertices);
    v3d_free_raw(&raw);
    v3d_png_free(&idat);
}


// --- 4. Render Logic ---
void PopulateCommandList() {
    g_commandAllocator->Reset();
    g_commandList->Reset(g_commandAllocator.Get(), g_pipelineState.Get());

    g_commandList->SetGraphicsRootSignature(g_rootSignature.Get());

    CameraCB cb = {};
    float mvp[16] = {
	    1,0,0,0,
	    0,1,0,0,
	    0,0,1,0,
	    0,0,-2,1
    };
    memcpy(cb.mvp,mvp,sizeof(mvp));
    memcpy(g_cameraPtr,&cb,sizeof(cb));

    g_commandList->SetGraphicsRootConstantBufferView(
		    0,
		    g_cameraBuffer->GetGPUVirtualAddress()
		    );


    // Viewport & Scissor (Manual)
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)g_width, (float)g_height, 0.0f, 1.0f };
    D3D12_RECT scissorRect = { 0, 0, (LONG)g_width, (LONG)g_height };
    g_commandList->RSSetViewports(1, &viewport);
    g_commandList->RSSetScissorRects(1, &scissorRect);

    // Barrier: Present -> Render Target
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = g_renderTargets[g_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    g_commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += (g_frameIndex * g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    
    // Clear Color (Martian Sunset Orange)
    const float clearColor[] = { 0.72f, 0.25f, 0.05f, 1.0f };
    g_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    g_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Draw
    g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
    g_commandList->DrawInstanced(
		    g_vertexBufferView.SizeInBytes/sizeof(Vertex),1,0,0
		    );

    // Barrier: Render Target -> Present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    g_commandList->ResourceBarrier(1, &barrier);

    g_commandList->Close();
}

void Sync() {
    g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
    g_fenceValue = 1;
    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void WaitForGPU() {
    const UINT64 fence = g_fenceValue;
    g_commandQueue->Signal(g_fence.Get(), fence);
    g_fenceValue++;

    if (g_fence->GetCompletedValue() < fence) {
        g_fence->SetEventOnCompletion(fence, g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, INFINITE);
    }
    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();
}
