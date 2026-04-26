#include "DevideManager.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <comdef.h>
#include <wrl.h>

using namespace Microsoft::WRL;

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


bool DeviceManager::Initialize(HWND hwnd)
{
#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

    // 1. Factory
    ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return false;

    // 2. Adapter
    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(0, &adapter)))
        return false;

    // 3. Device
    if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
        return false;

    // 4. Command Queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue))))
        return false;

    // 5. SwapChain
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.BufferCount = FRAME_COUNT;
    scDesc.Width = 1280;
    scDesc.Height = 720;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &scDesc, nullptr, nullptr, &swapChain1)))
        return false;

    if (FAILED(swapChain1.As(&swapChain)))
        return false;

    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // 6. RTV Heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FRAME_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 7. Render Targets
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < FRAME_COUNT; i++)
    {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize;
    }

    // 8. Command Allocators
    for (int i = 0; i < FRAME_COUNT; i++)
    {
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i]));
    }

    // 9. Command List
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&commandList));
    commandList->Close();

    // 10. Fence
    // Start with value 0
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

    // Initialize all values to 0
    for (int i = 0; i < FRAME_COUNT; i++)
        fenceValues[i] = 0;

    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!fenceEvent) return false;

    return true;
}

// This function now specifically waits for a fence value
void DeviceManager::WaitForFrame()
{
    // Wait until the GPU has reached the fence value for the current frame index
    if (fence->GetCompletedValue() < fenceValues[frameIndex])
    {
        fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

void DeviceManager::Signal()
{
    // Increment the value for the frame we just SUBMITTED
    fenceValues[frameIndex]++;
    DX_CHECK(commandQueue->Signal(fence.Get(), fenceValues[frameIndex]));
}

void DeviceManager::MoveToNextFrame()
{
    // 1. Move to the next backbuffer index
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // 2. WAIT for the NEW index to be ready.
    // If fenceValues[frameIndex] is 5, it means the last time we used this 
    // specific allocator, we told the GPU to signal '5' when done.
    // We cannot proceed until the GPU actually reaches '5'.
    if (fence->GetCompletedValue() < fenceValues[frameIndex])
    {
        DX_CHECK(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

// Keep this for when you need to force a total GPU flush (like in update)
void DeviceManager::FlushGPU()
{
    for (int i = 0; i < FRAME_COUNT; i++)
    {
        UINT64 valueToWait = fenceValues[i];
        if (fence->GetCompletedValue() < valueToWait)
        {
            fence->SetEventOnCompletion(valueToWait, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }
}

// Extra helper to clean up
void DeviceManager::Shutdown() {
    if (fenceEvent) {
        CloseHandle(fenceEvent);
    }
}