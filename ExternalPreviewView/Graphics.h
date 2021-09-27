#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>
#include <rpc.h>
#include <string>
#include <set>
#include <d3dcompiler.h>
#include <functional>
#include <chrono>
#include <wrl/client.h>
#include <d3d11_4.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcommon.h>
#include <DirectXMath.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")


template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
using namespace DirectX;

class Graphics
{
    // https://github.com/walbourn/directx-sdk-samples/tree/master/Direct3D11Tutorials
    HWND hwnd;
    ComPtr<IDXGIFactory2> factory;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain1> swapchain;
    ComPtr<ID3D11RenderTargetView> rtView;
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11InputLayout> vertexLayout;
    ComPtr<ID3D11ShaderResourceView> srView;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;

    HRESULT ThrowIfError(HRESULT hr)
    {
        if (hr != S_OK)
        {
            auto msg = _com_error(hr).ErrorMessage();
            throw std::wstring(msg);
        }
        return hr;
    }

public:
    static inline std::set<ComPtr<ID3D11Texture2D>> capturedTextures;
    static inline GUID GetPluginDxObjectDataGUID() { return { 0x09343630L, 0xaf9f, 0x11d0, { 0x92, 0x9f, 0x00, 0xc0, 0x4f, 0xc3, 0x40, 0xb1 } }; };

    inline Graphics(HWND hwnd) : hwnd(hwnd)
    {
#if _DEBUG
        ComPtr<ID3D12Debug> debug;
        ThrowIfError(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
        debug->EnableDebugLayer();
#endif

        ThrowIfError(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
        ThrowIfError(factory->EnumAdapters(0, &adapter));
        DXGI_ADAPTER_DESC adapterDesc;
        ThrowIfError(adapter->GetDesc(&adapterDesc));

        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_1 };
        D3D_FEATURE_LEVEL featureLevel;
        ThrowIfError(D3D11CreateDevice(
            adapter.Get(),
            D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
#if _DEBUG
            D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG,
#else
            0,
#endif
            featureLevels, _countof(featureLevels),
            D3D11_SDK_VERSION,
            &device, &featureLevel, &context));

        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {
            .Width = (UINT)(windowRect.right - windowRect.left),
            .Height = (UINT)(windowRect.bottom - windowRect.top),
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { 1, 0, },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 1,
        };
        ThrowIfError(factory->CreateSwapChainForHwnd(device.Get(), hwnd, &swapchainDesc, nullptr, nullptr, &swapchain));

        ComPtr<ID3D11Texture2D> swapchainBuffer;
        ThrowIfError(swapchain->GetBuffer(0, IID_PPV_ARGS(&swapchainBuffer)));
        //auto data = true;
        //ThrowIfError(swapchainBuffer->SetPrivateData(GetPluginDxObjectDataGUID(), sizeof(data), &data));

        ThrowIfError(device->CreateRenderTargetView(swapchainBuffer.Get(), nullptr, &rtView));

        XMFLOAT3 vertices[] =
        {
            XMFLOAT3(0.0f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.5f),
            XMFLOAT3(0.0f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.5f),
        };
        D3D11_BUFFER_DESC vertexBufferDesc = {
            .ByteWidth = sizeof(XMFLOAT3) * 6,
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = 0,
        };
        D3D11_SUBRESOURCE_DATA subResourceData = { .pSysMem = vertices };
        ThrowIfError(device->CreateBuffer(&vertexBufferDesc, &subResourceData, &vertexBuffer));


        ComPtr<ID3DBlob> vsBlob, psBlob, error;
        std::string code = 
            //"Texture2D t : register(t0);\n"
            //"SamplerState s : register(s0);\n"
            "float4 VS(float4 pos : POSITION) : SV_POSITION { return pos; }\n"
            //"float4 PS(float4 pos : SV_POSITION) : SV_Target { return t.Sample(s, pos.xy); };";
            "float4 PS(float4 pos : SV_POSITION) : SV_Target { return float4(1, 0, 0, 1); };";
        try
        {
            ThrowIfError(D3DCompile(code.data(), code.size(), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, &error));
            ThrowIfError(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs));
            ThrowIfError(D3DCompile(code.data(), code.size(), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, &error));
            ThrowIfError(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps));
        }
        catch (...)
        {
            throw std::exception((char*)error->GetBufferPointer());
        }

        D3D11_INPUT_ELEMENT_DESC layout[] = { { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, };
        ThrowIfError(device->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &vertexLayout));

        SetTimer(hwnd, 334, 1000 * 1 / 60, nullptr);
    }

    inline ~Graphics()
    {
        context->ClearState();
        context->Flush();
    }

    inline void Resize(int width, int height) {} // TODO:

    inline void Render()
    {
        context->OMSetRenderTargets(1, rtView.GetAddressOf(), nullptr);

        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        D3D11_VIEWPORT viewPort = {
            .TopLeftX = 0,
            .TopLeftY = 0,
            .Width = (FLOAT)(windowRect.right - windowRect.left),
            .Height = (FLOAT)(windowRect.bottom - windowRect.top),
            .MinDepth = 0,
            .MaxDepth = 1,
        };
        context->RSSetViewports(1, &viewPort);

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        FLOAT clearColor[] = { 0.0f, now % 1000 / 1000.0f, 0.0f, 1.0f };
        context->ClearRenderTargetView(rtView.Get(), clearColor);

        for (auto& texture : capturedTextures)
        {
            //texture->AddRef();
            //if (texture->Release() == 0)
            //{
            //    std::remove(texture, capturedTextures);
            //}

            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            OutputDebugStringW(std::format(L"{}x{} {}\n", desc.Width, desc.Height, (int)desc.Format).c_str());
        }
        static auto index = 0;
        index = (index + 1) % capturedTextures.size();
        auto texture = *std::next(capturedTextures.begin(), index);
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            .Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
            .ViewDimension = D3D11_SRV_DIMENSION::D3D10_1_SRV_DIMENSION_TEXTURE2D,
            .Texture2D = { .MostDetailedMip = 0, .MipLevels = 1, },
        };
        //ThrowIfError(device->CreateShaderResourceView(texture.Get(), &srvDesc, &srView)); // CreateTexture ‚ÉHook‚µ‚Ä D3D11_BIND_SHADER_RESOURCE ‚·‚ê‚Î’Ê‚é

        UINT stride = sizeof(XMFLOAT3);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetInputLayout(vertexLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        context->VSSetShader(vs.Get(), nullptr, 0);
        context->PSSetShader(ps.Get(), nullptr, 0);
        //context->PSSetShaderResources(0, 1, &srView);
        context->Draw(6, 0);

        swapchain->Present(0, 0);
    }
};
