#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>
#include "../HookHelper/HookHelper.h"
#include "Graphics.h"


const UINT WM_INITIALIZE_GRAPHICS = WM_USER + 334;
LRESULT MessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    std::function<void(HWND)> deleter = [](HWND h)
    {
        auto graphics = (Graphics*)GetWindowLongPtr(h, 0);
        if (graphics != nullptr)
        {
            delete graphics;
            SetWindowLongPtr(h, GWLP_USERDATA, (LONG_PTR)nullptr);
        }
    };

    try
    {
        if (message == WM_INITIALIZE_GRAPHICS)
        {
            auto graphics = new Graphics(hWnd);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)graphics);
        }
        else if (message == WM_SIZE)
        {
            auto graphics = (Graphics*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (graphics != nullptr)
            {
                graphics->Resize(LOWORD(lParam), HIWORD(lParam));
            }
        }
        else if (message == WM_TIMER)
        {
            RedrawWindow(hWnd, nullptr, nullptr, RDW_INTERNALPAINT);
            return 0;
        }
        else if (message == WM_PAINT)
        {
            auto graphics = (Graphics*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (graphics != nullptr)
            {
                graphics->Render();
                return 0;
            }
        }
        else if (message == WM_DESTROY)
        {
            deleter(hWnd);
            PostQuitMessage(0);
            return 0;
        }
    }
    catch (std::wstring& e)
    {
        deleter(hWnd);
        MessageBoxW(nullptr, e.c_str(), L"RecottePlugin", MB_ICONERROR);
        PostQuitMessage(0);
        return 0;
    }
    catch (std::exception& e)
    {
        deleter(hWnd);
        MessageBoxA(nullptr, e.what(), "RecottePlugin", MB_ICONERROR);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND InitWindow(HINSTANCE hInstance)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"ExternalPreviewWindow";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_APPLICATION);
    wcex.lpfnWndProc = MessageHandler;
    if (!RegisterClassEx(&wcex)) throw std::wstring(L"fail register");

    // Create window
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    auto hwnd = CreateWindow(L"ExternalPreviewWindow", L"RecottePlugin",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
        nullptr);
    if (!hwnd) throw std::wstring(L"fail create window");

    ShowWindow(hwnd, SW_SHOW);
    return hwnd;
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
    auto hwnd = InitWindow(handle);
    PostMessageW(hwnd, WM_INITIALIZE_GRAPHICS, 0, 0); // DllMainでDX系APIが使えないので遅延させる
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
