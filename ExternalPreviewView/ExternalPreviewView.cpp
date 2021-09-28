#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>
#include "resource.h"
#include "../HookHelper/HookHelper.h"
#include "Graphics.h"


const UINT WM_GRAPHICS_INITIALIZE = WM_USER + 114514;

decltype(&CreateWindowExW) g_Original_CreateWindowExW;
decltype(&D3D11CreateDevice) Original_D3D11CreateDevice;

std::function<void()> graphicCreatMessager;

HWND _CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto hwnd = g_Original_CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    if (lpWindowName != nullptr && std::wstring(lpWindowName) == L"1画面分進める")
    {
        auto hwnd = g_Original_CreateWindowExW(dwExStyle, L"BUTTON", L"uh_preview_popup", dwStyle, 1, 1, 26, 26, hWndParent, hMenu, hInstance, lpParam);
        WNDPROC proc = [](HWND hwnd, UINT m, WPARAM w, LPARAM l) -> LRESULT
        {
            static auto buttonPopupBitmap = LoadBitmap(GetModuleHandleW(L"ExternalPreviewView"), MAKEINTRESOURCE(BUTTON_POPUP));
            switch (m)
            {
            case WM_LBUTTONUP:
                //MessageBoxW(nullptr, L"プレビュー画面をポップアウトします", L"RecottePlugin", MB_OK);
                graphicCreatMessager();
                break;
            case WM_PAINT:
                PAINTSTRUCT ps;
                {
                    auto hdc = BeginPaint(hwnd, &ps);
                    auto memoryHdc = CreateCompatibleDC(hdc);
                    SetBkMode(hdc, TRANSPARENT);
                    SelectObject(memoryHdc, buttonPopupBitmap);
                    BitBlt(hdc, 0, 0, 26, 26, memoryHdc, 0, 0, SRCCOPY);
                    DeleteDC(memoryHdc);

                    EndPaint(hwnd, &ps);
                }
                break;
            case WM_DESTROY:
                DeleteObject(buttonPopupBitmap);
                break;
            }
            return DefWindowProcW(hwnd, m, w, l);
        };
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)proc);
    }
    return hwnd;
}

LRESULT MessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static std::shared_ptr<Graphics> graphics;
    try
    {
        switch (message)
        {
        case WM_GRAPHICS_INITIALIZE:
            if ((ID3D11DeviceContext*)wParam != nullptr)
            {
                graphics.reset(new Graphics(hWnd, (ID3D11DeviceContext*)wParam));
            }
            break;
        case WM_SIZE:
            if (graphics.get() != nullptr)
            {
                graphics->Resize();
            }
            break;
        case WM_TIMER:
            RedrawWindow(hWnd, nullptr, nullptr, RDW_INTERNALPAINT);
            break;
        case WM_LBUTTONDOWN:
            if (graphics.get() != nullptr)
            {
                graphics->OnClick(LOWORD(lParam), HIWORD(lParam));
            }
            break;
        case WM_PAINT:
            if (graphics.get() != nullptr)
            {
                graphics->Render();
            }
            break;
        case WM_DESTROY:
            graphics.reset();
            break;
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
    // register class
    WNDCLASSEX wcex;
    if (!GetClassInfoExW(hInstance, L"ExternalPreviewWindow", &wcex))
    {
        wcex = {
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = MessageHandler,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = hInstance,
            .hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_APPLICATION),
            .hCursor = LoadCursor(nullptr, IDC_ARROW),
            .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
            .lpszMenuName = nullptr,
            .lpszClassName = L"ExternalPreviewWindow",
            .hIconSm = LoadIcon(hInstance, (LPCTSTR)IDI_APPLICATION),
        };
        if (!RegisterClassEx(&wcex)) throw std::wstring(L"fail register");
    }

    // create window
    RECT rc = { 0, 0, 1600, 900 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    auto hwnd = CreateWindow(L"ExternalPreviewWindow", L"RecottePlugin",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
        nullptr);
    if (!hwnd) throw std::wstring(L"fail create window");

    ShowWindow(hwnd, SW_SHOW);
    //SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    return hwnd;
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
    static auto dllHandle = handle;
    g_Original_CreateWindowExW = RecottePluginFoundation::OverrideIATFunction("user32.dll", "CreateWindowExW", _CreateWindowExW);

    Original_D3D11CreateDevice = RecottePluginFoundation::OverrideIATFunction<decltype(&D3D11CreateDevice)>("d3d11.dll", "D3D11CreateDevice",
        [](IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext)
        {
            using Microsoft::WRL::ComPtr;
            typedef HRESULT(*CreateRenderTargetView)(ID3D11Device*, ID3D11Resource*, const D3D11_RENDER_TARGET_VIEW_DESC*, ID3D11RenderTargetView**);

#if _DEBUG
                Flags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
#endif

            auto hr = Original_D3D11CreateDevice(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);

            graphicCreatMessager = [context = *ppImmediateContext]()
            {
                auto hwnd = InitWindow(dllHandle);
                SendMessageW(hwnd, WM_GRAPHICS_INITIALIZE, (UINT_PTR)context, 0);
            };
            //graphicCreatMessager();

            auto vTable = *(CreateRenderTargetView**)(*ppDevice);
            DWORD oldrights, newrights = PAGE_READWRITE;
            VirtualProtect(&vTable[3 + 6], sizeof(CreateRenderTargetView), newrights, &oldrights);
            static auto originalCreateRenderTargetView = vTable[3 + 6];
            vTable[3 + 6] = [](ID3D11Device* device, ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView)
            {
                // TODO: 例外ケア
                Graphics::AddRenderTexture(pResource);
                return originalCreateRenderTargetView(device, pResource, pDesc, ppRTView);
            };
            return hr;
        });

}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
