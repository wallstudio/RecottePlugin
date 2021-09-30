#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>
#include <memory>
#include "resource.h"
#include "../HookHelper/HookHelper.h"
#include "PreviewWindow.h"

using Microsoft::WRL::ComPtr;

static std::shared_ptr<PreviewWindowBuilder> builder;

HWND _CreateWindowExW(decltype(&CreateWindowExW) base, DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    if (lpWindowName != nullptr && std::wstring(lpWindowName) == L"1画面分進める")
    {
        auto hwnd = base(dwExStyle, L"BUTTON", L"uh_preview_popup", dwStyle, 1, 1, 26, 26, hWndParent, hMenu, hInstance, lpParam);
        WNDPROC proc = [](HWND hwnd, UINT m, WPARAM w, LPARAM l) -> LRESULT
        {
            static auto buttonPopupBitmap = LoadBitmap(GetModuleHandleW(L"ExternalPreviewView"), MAKEINTRESOURCE(BUTTON_POPUP));
            switch (m)
            {
            case WM_LBUTTONUP:
                builder->Create();
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

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
    static auto module = handle;

    // PreviewWidnowの機能ボタン
    static decltype(&CreateWindowExW) createWindowExW = RecottePluginFoundation::OverrideIATFunction<decltype(&CreateWindowExW)>(
        "user32.dll", "CreateWindowExW",
        [](DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
        {
            return _CreateWindowExW(createWindowExW, dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        });

    // DX Deviceを掠め取る
    static decltype(&D3D11CreateDevice) d3D11CreateDevice = RecottePluginFoundation::OverrideIATFunction<decltype(&D3D11CreateDevice)>(
        "d3d11.dll", "D3D11CreateDevice",
        [](IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
            ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext) -> HRESULT
        {
            typedef HRESULT(*CreateRenderTargetView)(ID3D11Device*, ID3D11Resource*, const D3D11_RENDER_TARGET_VIEW_DESC*, ID3D11RenderTargetView**);
#if _DEBUG
                Flags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
#endif
            auto hr = d3D11CreateDevice(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
            static ID3D11DeviceContext* context = *ppImmediateContext;

            // レンダーテクスチャ作成時に、それをを掠め取る
            auto vTable = *(CreateRenderTargetView**)(*ppDevice);
            static CreateRenderTargetView createRenderTargetView = vTable[3 + 6]; // CINTERFACE にしたい
            auto proc = (CreateRenderTargetView)[](ID3D11Device* device, ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView)
            {
                auto hr = createRenderTargetView(device, pResource, pDesc, ppRTView);
                if (builder == nullptr && Graphics::CheckCapturableRTV(*ppRTView))
                {
                    builder.reset(new PreviewWindowBuilder(module, device, context, *ppRTView));
                }
                return hr;
            };
            RecottePluginFoundation::MemoryCopyAvoidingProtection(&vTable[3 + 6], &proc, sizeof(CreateRenderTargetView));
            return hr;
        });
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
