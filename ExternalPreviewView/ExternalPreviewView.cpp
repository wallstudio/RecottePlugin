﻿#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>
#include "../HookHelper/HookHelper.h"
#include "Graphics.h"


const UINT WM_INITIALIZE_GRAPHICS = WM_USER + 334;

decltype(&D3D11CreateDevice) Original_D3D11CreateDevice;
typedef HRESULT(*CreateRenderTargetView)(ID3D11Device*, ID3D11Resource*, const D3D11_RENDER_TARGET_VIEW_DESC*, ID3D11RenderTargetView**);
CreateRenderTargetView originalCreateRenderTargetView;
std::shared_ptr<Graphics> graphics;
HWND hwnd;

LRESULT MessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    try
    {
        if (message == WM_INITIALIZE_GRAPHICS)
        {
            //graphics.reset(new Graphics(hWnd));
        }
        else if (message == WM_SIZE)
        {
            if (graphics.get() != nullptr)
            {
                graphics->Resize(LOWORD(lParam), HIWORD(lParam));
            }
        }
        else if (message == WM_TIMER)
        {
            RedrawWindow(hWnd, nullptr, nullptr, RDW_INTERNALPAINT);
            return 0;
        }
        else if (message == WM_LBUTTONDOWN)
        {
            if (graphics.get() != nullptr)
            {
                graphics->index++;
            }
            return 0;
        }
        else if (message == WM_PAINT)
        {
            if (graphics.get() != nullptr)
            {
                graphics->Render();
                return 0;
            }
        }
        else if (message == WM_DESTROY)
        {
            graphics.reset();
            PostQuitMessage(0);
            return 0;
        }
    }
    catch (std::wstring& e)
    {
        graphics.reset();
        MessageBoxW(nullptr, e.c_str(), L"RecottePlugin", MB_ICONERROR);
        PostQuitMessage(0);
        return 0;
    }
    catch (std::exception& e)
    {
        graphics.reset();
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
    // NOTE: Swapchainは使っていない（プログラム内の参照は画面キャプチャの時用）

    Original_D3D11CreateDevice = RecottePluginFoundation::OverrideIATFunction<decltype(&D3D11CreateDevice)>("d3d11.dll", "D3D11CreateDevice",
        [](IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext)
        {
            using Microsoft::WRL::ComPtr;

            #if _DEBUG
            Flags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
            #endif
            auto hr = Original_D3D11CreateDevice(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
            graphics.reset(new Graphics(hwnd, *ppDevice, *ppImmediateContext));

            auto vTable = *(CreateRenderTargetView**)(*ppDevice);
            DWORD oldrights, newrights = PAGE_READWRITE;
            VirtualProtect(&vTable[3 + 6], sizeof(CreateRenderTargetView), newrights, &oldrights);
            originalCreateRenderTargetView = vTable[3 + 6];
            vTable[3 + 6] = [](ID3D11Device* device, ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView)
            {
                auto hr = originalCreateRenderTargetView(device, pResource, pDesc, ppRTView);
                if (graphics.get() != nullptr)
                {
                    graphics->AddRenderTexture(pResource);
                }
                return hr;
            };
            return hr;
        });

    hwnd = InitWindow(handle);
    PostMessageW(hwnd, WM_INITIALIZE_GRAPHICS, 0, 0); // DllMainでDX系APIが使えないので遅延させる
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
