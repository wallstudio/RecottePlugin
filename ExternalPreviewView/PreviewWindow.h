﻿#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <set>
#include <random>
#include <functional>
#include "Graphics.h"


class Window
{
public:
    typedef std::function<LRESULT(UINT, WPARAM, LPARAM)> MessageHandlerCallback;

protected:
    static inline RECT rc = { 0, 0, 1600, 900 };
    HWND hwnd;
    std::shared_ptr<MessageHandlerCallback> messageHandler;

    inline virtual void MessageHandlerImpl(UINT message, WPARAM wParam, LPARAM lParam, LRESULT*& result)
    {
        switch (message)
        {
        case WM_MOVE:
        case WM_SIZE:
            GetWindowRect(hwnd, &rc);
            break;
        case WM_DESTROY:
            if (onDestroy != nullptr)
            {
                onDestroy();
            }
            break;
        }
    }

    inline LRESULT MessageHandler(UINT message, WPARAM wParam, LPARAM lParam)
    {
        try
        {
            LRESULT* result = nullptr;
            MessageHandlerImpl(message, wParam, lParam, result);
            if (result != nullptr)
            {
                return *result;
            }
        }
        catch (std::wstring& e)
        {
            MessageBoxW(nullptr, e.c_str(), L"RecottePlugin", MB_ICONERROR);
            PostQuitMessage(0);
            return 0;
        }
        catch (std::exception& e)
        {
            MessageBoxA(nullptr, e.what(), "RecottePlugin", MB_ICONERROR);
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

public:
    std::function<void()> onDestroy;

    inline Window(const std::wstring windowClassName)
    {
        hwnd = CreateWindowW(windowClassName.c_str(), L"RecottePlugin",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, GetModuleHandleW(nullptr),
            nullptr);
        if (!hwnd) throw std::wstring(L"fail create window");

        messageHandler.reset(new MessageHandlerCallback([&](UINT m, WPARAM w, LPARAM l) -> LRESULT { return MessageHandler(m, w, l); }));
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)messageHandler.get());

        SetWindowPos(hwnd, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
        ShowWindow(hwnd, SW_SHOW);
    }

    inline virtual ~Window() { SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)nullptr); }

    static inline MessageHandlerCallback GetMessageHandler(HWND hwnd)
    {
        auto handler = (MessageHandlerCallback*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        return handler != nullptr ? *handler : nullptr;
    }

};


class PreviewWindow : public Window
{
    std::shared_ptr<Graphics> graphics;

    inline void MessageHandlerImpl(UINT message, WPARAM wParam, LPARAM lParam, LRESULT*& result) override
    {
        switch (message)
        {
        case WM_SIZE:
            graphics->Resize();
            break;
        case WM_PAINT:
            graphics->Render();
            break;
        case WM_LBUTTONDOWN:
            graphics->OnClick(LOWORD(lParam), HIWORD(lParam));
            break;
        }
        Window::MessageHandlerImpl(message, wParam, lParam, result);
    }

public:
    inline PreviewWindow(const std::wstring windowClassName, ComPtr<ID3D11RenderTargetView> rtv) : Window(windowClassName)
    {
        graphics.reset(new Graphics(hwnd, rtv));
        //SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    inline void Present() { RedrawWindow(hwnd, nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE); }
};


class PreviewWindowBuilder
{
    static inline std::random_device rand;
    const ComPtr<ID3D11RenderTargetView> rtv;
    std::wstring windowClassName;
    std::set<std::shared_ptr<PreviewWindow>> instances;

public:
    inline PreviewWindowBuilder(const ComPtr<ID3D11RenderTargetView> rtv) : rtv(rtv)
    {
        windowClassName = std::format(L"ExternalPreviewWindow_{:08x}", rand());
        WNDCLASSEXW wcex = {
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = [](HWND h, UINT m, WPARAM w, LPARAM l) {
                auto handler = Window::GetMessageHandler(h);
                return handler != nullptr ? handler(m, w, l) : DefWindowProcW(h, m, w, l);
            },
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = GetModuleHandleW(nullptr),
            .hIcon = LoadIcon(GetModuleHandleW(nullptr), (LPCTSTR)IDI_APPLICATION),
            .hCursor = LoadCursor(nullptr, IDC_ARROW),
            .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
            .lpszMenuName = nullptr,
            .lpszClassName = windowClassName.c_str(),
            .hIconSm = LoadIcon(GetModuleHandleW(nullptr), (LPCTSTR)IDI_APPLICATION),
        };
        if (!RegisterClassExW(&wcex)) throw std::wstring(L"fail register");
    }

    inline std::shared_ptr<PreviewWindow> Create()
    {
        auto window = std::shared_ptr<PreviewWindow>(new PreviewWindow(windowClassName, rtv));
        instances.insert(window);
        window->onDestroy = [&]() { instances.erase(window); };
        return window;
    }

    inline void PresentSingal()
    {
        for (auto& instance : instances)
        {
            instance->Present();
        }
    }
    
};

