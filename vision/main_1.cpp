#include <windows.h>
#include <stdio.h>

// DX12 core
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

// =========================
// EXTERNALS FROM dx12_init.cpp
// =========================
extern void InitD3D12(HWND hwnd);
extern void CreatePipeline();
extern void CreateVertexBuffer();
extern void PopulateCommandList();
extern void WaitForGPU();
extern void Sync();

// These MUST match dx12_init.cpp globals
extern ComPtr<ID3D12CommandQueue> g_commandQueue;
extern ComPtr<IDXGISwapChain3> g_swapChain;
extern ComPtr<ID3D12GraphicsCommandList> g_commandList;

// =========================
// WINDOW PROCEDURE
// =========================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// =========================
// ENTRY POINT
// =========================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    printf("=== V3D DX12 SYMBOLIC PIPELINE ===\n");

    // -------------------------
    // 1. REGISTER WINDOW CLASS
    // -------------------------
    const char CLASS_NAME[] = "V3DWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // -------------------------
    // 2. CREATE WINDOW
    // -------------------------
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "V3D Symbolic Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd)
    {
        printf("❌ Failed to create window\n");
        return -1;
    }

    ShowWindow(hwnd, nCmdShow);

    // -------------------------
    // 3. INIT DX12 PIPELINE
    // -------------------------
    InitD3D12(hwnd);
    CreatePipeline();
    CreateVertexBuffer();
    Sync();

    printf("✅ DX12 Initialized\n");

    // -------------------------
    // 4. MAIN LOOP
    // -------------------------
    MSG msg = {};

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Record commands
            PopulateCommandList();

            // Execute
            ID3D12CommandList* lists[] = { g_commandList.Get() };
            g_commandQueue->ExecuteCommandLists(1, lists);

            // Present
            g_swapChain->Present(1, 0);

            // Sync GPU
            WaitForGPU();
        }
    }

    return 0;
}
