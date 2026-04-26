#include "framework.h"
#include "smallEngine.h"
#include "DevideManager.h"
#include "Rendrer.h"

#define MAX_LOADSTRING 100

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

// Graphics Globals
DeviceManager g_DeviceManager;
Rendrer g_Renderer;

// Mouse Tracking
bool isMouseCaptured = true;

// Forward declarations
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, HWND&);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SMALLENGINE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    HWND hWnd = nullptr;
    if (!InitInstance(hInstance, nCmdShow, hWnd))
    {
        return FALSE;
    }

    // --- 1. INITIALIZE D3D12 ---
    if (!g_DeviceManager.Initialize(hWnd)) {
        OutputDebugStringA("Failed to Initialize Device Manager\n");
        return FALSE;
    }
    g_Renderer.Init(&g_DeviceManager);

    // Initial mouse setup
    if (isMouseCaptured) ShowCursor(FALSE);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SMALLENGINE));
    MSG msg = {};

    // --- 2. THE GAME LOOP ---
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            // --- 3. ROBUST MOUSE HANDLING ---
            // Only handle mouse if captured and the window is actually in the foreground
            if (isMouseCaptured && GetForegroundWindow() == hWnd) {
                RECT rect;
                GetWindowRect(hWnd, &rect);
                int centerX = rect.left + (rect.right - rect.left) / 2;
                int centerY = rect.top + (rect.bottom - rect.top) / 2;

                POINT currentPos;
                GetCursorPos(&currentPos);

                float dx = (float)(currentPos.x - centerX);
                float dy = (float)(currentPos.y - centerY);

                // Only trigger update if there is actual movement to avoid jitter
                if (dx != 0 || dy != 0) {
                    g_Renderer.sc.player.HandleMouse(dx, dy);
                    SetCursorPos(centerX, centerY);
                }
            }

            // --- 4. RUN THE ENGINE ---
            g_Renderer.update(&g_DeviceManager);
            g_Renderer.render(&g_DeviceManager);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALLENGINE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hWndOut)
{
    hInst = hInstance;
    // Windowed mode at 1280x720
    hWndOut = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 1280, 720, nullptr, nullptr, hInstance, nullptr);

    if (!hWndOut) return FALSE;

    ShowWindow(hWndOut, nCmdShow);
    UpdateWindow(hWndOut);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        // ESC to toggle mouse capture
        if (wParam == VK_ESCAPE) {
            isMouseCaptured = !isMouseCaptured;
            ShowCursor(!isMouseCaptured);
        }
        break;
    case WM_SETFOCUS:
        if (isMouseCaptured) ShowCursor(FALSE);
        break;
    case WM_KILLFOCUS:
        // Always show cursor if we alt-tab out
        ShowCursor(TRUE);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}