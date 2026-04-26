#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

class DeviceManager
{
public:
    static const int FRAME_COUNT = 10;

    bool Initialize(HWND hwnd);

    // Basic Getters
    ID3D12Device* GetDevice() { return device.Get(); }
    IDXGISwapChain3* GetSwapChain() { return swapChain.Get(); }
    ID3D12CommandQueue* GetCommandQueue() { return commandQueue.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() { return commandList.Get(); }

    // Returns the index of the backbuffer currently being processed (0 or 1)
    UINT GetCurrentFrameIndex() { return frameIndex; }

    // Allocator Getters
    ID3D12CommandAllocator* GetCurrentAllocator() {
        return commandAllocators[frameIndex].Get();
    }

    // Added this helper to fix the E0135 error in Rendrer.cpp
    ID3D12CommandAllocator* GetCommandAllocator(UINT index) {
        return commandAllocators[index].Get();
    }

    // Resource Getters
    ID3D12Resource* GetRenderTarget(UINT i) { return renderTargets[i].Get(); }
    ID3D12DescriptorHeap* GetRtvHeap() { return rtvHeap.Get(); }
    UINT GetRtvDescriptorSize() { return rtvDescriptorSize; }

    // Synchronization
    void WaitForFrame();   // Waits for the current frameIndex to be ready
    void Signal();         // Signals the fence for the current frame
    void MoveToNextFrame();// Switches index and waits for the NEW index
    void FlushGPU();       // Hard stop: waits for everything to finish
    void Shutdown();

private:
    ComPtr<ID3D12Device> device;
    ComPtr<IDXGISwapChain3> swapChain;
    ComPtr<ID3D12CommandQueue> commandQueue;

    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    UINT rtvDescriptorSize;

    ComPtr<ID3D12Resource> renderTargets[FRAME_COUNT];

    // Command Management
    ComPtr<ID3D12CommandAllocator> commandAllocators[FRAME_COUNT];
    ComPtr<ID3D12GraphicsCommandList> commandList;

    // Fence System
    ComPtr<ID3D12Fence> fence;
    UINT64 fenceValues[FRAME_COUNT] = { 0, 0 };
    HANDLE fenceEvent;

    UINT frameIndex = 0;
};