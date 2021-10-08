#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>
#include <memory>
#include "resource.h"
#include "../RecottePluginManager/PluginHelper.h"
#include "PreviewWindow.h"

using Microsoft::WRL::ComPtr;

static std::shared_ptr<PreviewWindowBuilder> builder;

HWND _CreateWindowExW(decltype(&CreateWindowExW) base, DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    if (lpWindowName != nullptr && std::wstring(lpWindowName) == L"1画面分進める")
    {
        // PreviewWidnowの機能ボタン
        auto hwnd = base(dwExStyle, L"BUTTON", L"uh_preview_popup", dwStyle, 1, 1, 26, 26, hWndParent, hMenu, hInstance, lpParam);
        WNDPROC proc = [](HWND hwnd, UINT m, WPARAM w, LPARAM l) -> LRESULT
        {
            static auto buttonPopupIcon= LoadIconW(GetModuleHandleW(L"ExternalPreviewView"), MAKEINTRESOURCE(BUTTON_POPUP));
            static auto background = CreateSolidBrush(0x3e3e3e);
            static auto hoverBackground = CreateSolidBrush(0x505050);
            static auto pressedBackground = CreateSolidBrush(0x202020);
            static auto hover = false;
            static auto pressed = false;
            switch (m)
            {
            case WM_LBUTTONUP:
                builder->Create();
                break;
            case WM_PAINT:
                {
                    PAINTSTRUCT ps;
                    auto hdc = BeginPaint(hwnd, &ps);
                    static RECT rect = { 0, 0, 26, 26 };
                    FillRect(hdc, &rect, pressed ? pressedBackground : (hover ? hoverBackground : background));
                    auto u = DrawIconEx(hdc, 0, 0, buttonPopupIcon, 26, 26, 0, nullptr, DI_NORMAL | DI_COMPAT);
                    EndPaint(hwnd, &ps);
                }
                break;
            case WM_LBUTTONDOWN:
            case WM_MOUSEMOVE:
                {
                    pressed = w == MK_LBUTTON;
                    TRACKMOUSEEVENT track = {
                        .cbSize = sizeof(TRACKMOUSEEVENT),
                        .dwFlags = TME_HOVER | TME_LEAVE,
                        .hwndTrack = hwnd,
                        .dwHoverTime = 1,
                    };
                    TrackMouseEvent(&track);
                    RedrawWindow(hwnd, nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
            }
                break;
            case WM_MOUSEHOVER:
                hover = true;
                RedrawWindow(hwnd, nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
                break;
            case WM_MOUSELEAVE:
                hover = pressed = false;
                RedrawWindow(hwnd, nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
                break;
            case WM_DESTROY:
                DeleteObject(buttonPopupIcon);
                DeleteObject(background);
                DeleteObject(hoverBackground);
                DeleteObject(pressedBackground);
                break;
            }
            return DefWindowProcW(hwnd, m, w, l);
        };
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)proc);
    }
    return hwnd;
}

HRESULT _D3D11CreateDevice(decltype(&D3D11CreateDevice) base,
    IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
    ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext)
{
#if _DEBUG
    Flags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
#endif

    // DX Deviceを掠め取る
    auto hr = base(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
    static ID3D11DeviceContext* context = *ppImmediateContext;

    // レンダーテクスチャ作成時に、それをを掠め取る
    typedef HRESULT(*CreateRenderTargetView)(ID3D11Device*, ID3D11Resource*, const D3D11_RENDER_TARGET_VIEW_DESC*, ID3D11RenderTargetView**);
    auto vTable = *(CreateRenderTargetView**)(*ppDevice);
    static CreateRenderTargetView createRenderTargetView = vTable[3 + 6]; // CINTERFACE にしたい

    auto proc = (CreateRenderTargetView)[](ID3D11Device* device, ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView)
    {
        auto hr = createRenderTargetView(device, pResource, pDesc, ppRTView);
        if (builder == nullptr && Graphics::CheckCapturableRTV(*ppRTView))
        {
            builder.reset(new PreviewWindowBuilder(*ppRTView));
        }
        return hr;
    };
    RecottePluginManager::MemoryCopyAvoidingProtection(&vTable[3 + 6], &proc, sizeof(CreateRenderTargetView));

    {
        // SwapChaine::Present1のタイミングを掠め取る
        typedef HRESULT(*QueryInterface)(IUnknown* This, REFIID riid, void** ppvObject);
        auto vTable = *(QueryInterface**)(*ppDevice);
        static QueryInterface queryInterface = vTable[0];
        auto proc = (QueryInterface)[](IUnknown* This, REFIID riid, void** ppvObject) -> HRESULT
        {
            auto hr = queryInterface(This, riid, ppvObject);
            if (riid != __uuidof(ID3D11Device1)) return hr;

            auto vTable = *(QueryInterface**)(*ppvObject);
            static QueryInterface queryInterface = vTable[0];
            auto proc = (QueryInterface)[](IUnknown* This, REFIID riid, void** ppvObject) -> HRESULT
            {
                auto hr = queryInterface(This, riid, ppvObject);
                if (riid != __uuidof(IDXGIDevice1)) return hr;

                typedef HRESULT(*GetAdapter)(IDXGIDevice1* This, IDXGIAdapter** pAdapter);
                auto vTable = *(GetAdapter**)(*ppvObject);
                static GetAdapter getAdapter = vTable[7];
                auto proc = (GetAdapter)[](IDXGIDevice1* This, IDXGIAdapter** pAdapter)->HRESULT
                {
                    auto hr = getAdapter(This, pAdapter);

                    typedef HRESULT(*GetParent)(IDXGIAdapter1* This, REFIID riid, void** ppParent);
                    auto vTable = *(GetParent**)(*pAdapter);
                    static GetParent getParent = vTable[6];
                    auto proc = (GetParent)[](IDXGIAdapter1* This, REFIID riid, void** ppParent)->HRESULT
                    {
                        auto hr = getParent(This, riid, ppParent);
                        if (riid != __uuidof(IDXGIFactory2)) return hr;

                        typedef HRESULT(*CreateSwapChainForHwnd)(IDXGIFactory2* This, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain);
                        auto vTable = *(CreateSwapChainForHwnd**)(*ppParent);
                        static CreateSwapChainForHwnd createSwapchain = vTable[15];
                        auto proc = (CreateSwapChainForHwnd)[](IDXGIFactory2* This, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)->HRESULT
                        {
                            auto hr = createSwapchain(This, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);

                            typedef HRESULT(*Present1)(IDXGISwapChain1* This, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters);
                            auto vTable = *(Present1**)(*ppSwapChain);
                            static Present1 present = vTable[22];
                            auto proc = (Present1)[](IDXGISwapChain1* This, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)->HRESULT
                            {
                                auto hr = present(This, SyncInterval, PresentFlags, pPresentParameters);
                                if (builder != nullptr)
                                {
                                    builder->PresentSingal();
                                }
                                return hr;
                            };
                            RecottePluginManager::MemoryCopyAvoidingProtection(&vTable[22], &proc, sizeof(Present1));
                            return hr;
                        };
                        RecottePluginManager::MemoryCopyAvoidingProtection(&vTable[15], &proc, sizeof(CreateSwapChainForHwnd));
                        return hr;
                    };
                    RecottePluginManager::MemoryCopyAvoidingProtection(&vTable[6], &proc, sizeof(GetParent));
                    return hr;
                };
                RecottePluginManager::MemoryCopyAvoidingProtection(&vTable[7], &proc, sizeof(GetAdapter));
                return hr;
            };
            RecottePluginManager::MemoryCopyAvoidingProtection(&vTable[0], &proc, sizeof(QueryInterface));
            return hr;
        };
        RecottePluginManager::MemoryCopyAvoidingProtection(&vTable[0], &proc, sizeof(QueryInterface));
        return hr;
    }
    return hr;
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
    static decltype(&CreateWindowExW) createWindowExW = RecottePluginManager::OverrideIATFunction<decltype(&CreateWindowExW)>("user32.dll", "CreateWindowExW", [](auto ...args) { return _CreateWindowExW(createWindowExW, args...); });
    static decltype(&D3D11CreateDevice) d3D11CreateDevice = RecottePluginManager::OverrideIATFunction<decltype(&D3D11CreateDevice)>("d3d11.dll", "D3D11CreateDevice", [](auto ...args) { return _D3D11CreateDevice(d3D11CreateDevice, args...); });
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
