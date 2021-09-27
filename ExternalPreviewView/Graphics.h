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


using Microsoft::WRL::ComPtr;
using namespace DirectX;


class Graphics
{
    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
        XMFLOAT4 uv;
    };

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
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11Buffer> cBuffer;
    std::set<ComPtr<ID3D11Texture2D>> capturedTextures;
    size_t index = 0;

    static inline HRESULT ThrowIfError(HRESULT hr)
    {
        if (hr != S_OK)
        {
            auto msg = _com_error(hr).ErrorMessage();
            throw std::format(L"{}\n0x{:08x}", msg, hr);
        }
        return hr;
    }

    static inline void CreateShader(ComPtr<ID3D11Device> device, ComPtr<ID3D11VertexShader>& vs, ComPtr<ID3D11PixelShader>& ps, ComPtr<ID3DBlob>& vsBlob)
    {
        ComPtr<ID3DBlob> psBlob, error;
        char code[] = R"(
Texture2D _MainTex : register(t0);
SamplerState _MainTexSampler : register(s0);
cbuffer _CB : register(b0)
{
    float4x4 transform;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float4 uv : TEXCOORD0;
};
struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float4 uv : TEXCOORD0;
};

PS_INPUT VS( VS_INPUT input )
{
    // TODO: MVP?
    PS_INPUT output;
    output.position = mul(float4(input.position, 1), transform);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}
float4 PS( PS_INPUT input) : SV_Target
{
    float4 color = float4(1, 1, 1, 1);
    color *= input.color;
    color *= _MainTex.Sample(_MainTexSampler, input.uv.xy);
    return color;
}
        )";
        try
        {
            ThrowIfError(D3DCompile(code, sizeof(code), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, &error));
            ThrowIfError(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs));
            ThrowIfError(D3DCompile(code, sizeof(code), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, &error));
            ThrowIfError(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps));
        }
        catch (...)
        {
            throw std::exception((char*)error->GetBufferPointer());
        }
    }

    static inline void CreateVertex(ComPtr<ID3D11Device> device, ComPtr<ID3DBlob> vsBlob, ComPtr<ID3D11Buffer>& vertexBuffer, ComPtr<ID3D11SamplerState>& sampler, ComPtr<ID3D11InputLayout>& vertexLayout)
    {
        Vertex vertices[] = {
            { XMFLOAT3(-1, -1, 0.5f), XMFLOAT4(1, 1, 1, 1), XMFLOAT4(-0, +1, 0, 0) },
            { XMFLOAT3(-1, +1, 0.5f), XMFLOAT4(1, 1, 1, 1), XMFLOAT4(-0, -0, 0, 0) },
            { XMFLOAT3(+1, +1, 0.5f), XMFLOAT4(1, 1, 1, 1), XMFLOAT4(+1, -0, 0, 0) },

            { XMFLOAT3(-1, -1, 0.5f), XMFLOAT4(1, 1, 1, 1), XMFLOAT4(-0, +1, 0, 0) },
            { XMFLOAT3(+1, +1, 0.5f), XMFLOAT4(1, 1, 1, 1), XMFLOAT4(+1, -0, 0, 0) },
            { XMFLOAT3(+1, -1, 0.5f), XMFLOAT4(1, 1, 1, 1), XMFLOAT4(+1, +1, 0, 0) },
        };
        D3D11_BUFFER_DESC vertexBufferDesc = {
            .ByteWidth = sizeof(vertices),
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = 0,
        };
        D3D11_SUBRESOURCE_DATA subResourceData = { .pSysMem = vertices };
        ThrowIfError(device->CreateBuffer(&vertexBufferDesc, &subResourceData, &vertexBuffer));

        D3D11_SAMPLER_DESC sampDesc = {
            .Filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            .AddressU = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressV = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
            .ComparisonFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER,
            .MinLOD = 0,
            .MaxLOD = D3D11_FLOAT32_MAX,
        };
        device->CreateSamplerState(&sampDesc, &sampler);

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3) + sizeof(XMFLOAT4), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        }; 
        ThrowIfError(device->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &vertexLayout));
    }

    static inline void CreateConstant(ComPtr<ID3D11Device> device, ComPtr<ID3D11Buffer>& cBuffer)
    {
        D3D11_BUFFER_DESC desc =
        {
            .ByteWidth = sizeof(XMMATRIX),
            .Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
        };
        ThrowIfError(device->CreateBuffer(&desc, nullptr, &cBuffer));
    }

public:
    inline Graphics(HWND hwnd, ID3D11DeviceContext* context) : hwnd(hwnd), context(context)
    {
#if _DEBUG
        ComPtr<ID3D12Debug> debug;
        ThrowIfError(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
        debug->EnableDebugLayer();
#endif
        context->GetDevice(&device);
        ComPtr<IDXGIDevice1> dxgiDevice;
        ThrowIfError(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
        ThrowIfError(dxgiDevice->GetAdapter(adapter.GetAddressOf()));
        ThrowIfError(adapter->GetParent(__uuidof(IDXGIFactory1), &factory));

        ComPtr<ID3DBlob> vsBlob;
        CreateShader(device, vs, ps, vsBlob);
        CreateVertex(device, vsBlob, vertexBuffer, sampler, vertexLayout);
        CreateConstant(device, cBuffer);

        Resize();
               
        SetTimer(hwnd, 334, 1000 * 1 / 60, nullptr);
    }

    inline ~Graphics()
    {
        context->ClearState();
        context->Flush();
    }

    inline void Resize()
    {
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
        ThrowIfError(device->CreateRenderTargetView(swapchainBuffer.Get(), nullptr, &rtView));

        if (capturedTextures.size())
        {
            auto texture = *std::next(capturedTextures.begin(), index);
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            auto texAspect = desc.Width / (double)desc.Height;
            auto winAspect = swapchainDesc.Width / (double)swapchainDesc.Height;
            FXMMATRIX matrix = winAspect > texAspect
                ? XMMatrixScaling(1 / winAspect * texAspect, 1, 1)
                : XMMatrixScaling(1, 1 * winAspect / texAspect, 1);
            context->UpdateSubresource(cBuffer.Get(), 0, nullptr, &matrix, 0, 0);
        }
    }

    inline void OnClick(int width, int height)
    {
        if (capturedTextures.size() > 0)
        {
            index = (index + 1) % capturedTextures.size();
            Resize();
        }
    }

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

        static FLOAT clearColor[] = { 0, 0, 0, 1 };
        context->ClearRenderTargetView(rtView.Get(), clearColor);

        UINT stride = sizeof(Vertex), offset = 0;
        context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetInputLayout(vertexLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        if (capturedTextures.size())
        {
            for (auto& texture : capturedTextures)
            {
                texture->AddRef();
                if (texture->Release() == 1)
                {
                    // TODO: 本体が掴んでいるInterfaceがこれじゃないのでこれじゃダメ（このリークが避けられない…）
                    //capturedTextures.erase(texture);
                }
            }
            auto texture = *std::next(capturedTextures.begin(), index);
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {
                .Format = desc.Format,
                .ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D,
                .Texture2D = {.MostDetailedMip = 0, .MipLevels = 1, },
            };
            ThrowIfError(device->CreateShaderResourceView(texture.Get(), &srvDesc, &srView));
            context->PSSetShaderResources(0, 1, srView.GetAddressOf());
            context->PSSetSamplers(0, 1, sampler.GetAddressOf());
            context->VSSetConstantBuffers(0, 1, cBuffer.GetAddressOf());
            context->VSSetShader(vs.Get(), nullptr, 0);
            context->PSSetShader(ps.Get(), nullptr, 0);
            context->Draw(6, 0);
        }

        auto presentResult = swapchain->Present(0, 0);
        if (presentResult != DXGI_STATUS_OCCLUDED) // Minimize
        {
            ThrowIfError(presentResult);
        }

        context->PSSetShaderResources(0, 0, nullptr);
        context->Flush();
    }

    inline void AddRenderTexture(ComPtr<ID3D11Resource> resource)
    {
        ComPtr<ID3D11Texture2D> texture;
        if (FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture))))
        {
            OutputDebugStringW(std::format(L"not Texture2D\n").c_str());
            return;
        }

        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc(&desc);
        if ((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != D3D11_BIND_SHADER_RESOURCE)
        {
            OutputDebugStringW(std::format(L"not RenderTexture {}x{} {}\n", desc.Width, desc.Height, (int)desc.Format).c_str());
            return;
        }
        if (desc.Height != 1080) return;

        capturedTextures.insert(texture);
    }
};
