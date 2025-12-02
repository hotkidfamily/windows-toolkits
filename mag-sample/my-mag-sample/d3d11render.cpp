#include "stdafx.h"

#include "d3d11render.h"
#include "cpgpu.h"

#include <chrono>
#include <directxmath.h>

#pragma comment(lib, "d3d11.lib")

using namespace Microsoft::WRL;

d3d11render::~d3d11render()
{
    constexpr float ClearColor[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
    if (_ctx && _mainRTV && _swapChain) {
        _ctx->ClearRenderTargetView(_mainRTV.Get(), ClearColor);
        _swapChain->Present(0, 0);
    }
}

HRESULT d3d11render::init(HWND hwnd)
{
    HRESULT hResult = S_OK;

    CRect rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.Width();
    UINT height = rc.Height();

    _hwndSz = { rc.Width(), rc.Height() };
    _hwndSettingSz = _hwndSz;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[]
        = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(DXGI_SWAP_CHAIN_DESC));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Windowed = TRUE;

    D3D_DRIVER_TYPE driverTypeIdx = D3D_DRIVER_TYPE_UNKNOWN;
    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex) {
        driverTypeIdx = driverTypes[driverTypeIndex];
        hResult = D3D11CreateDeviceAndSwapChain(
            NULL,
            driverTypeIdx,
            NULL,
            createDeviceFlags,
            featureLevels,
            numFeatureLevels,
            D3D11_SDK_VERSION, 
            &sd, &_swapChain, &_dev, &_featureLevel, &_ctx);
        if (SUCCEEDED(hResult))
            break;
    }
    if (FAILED(hResult))
        return hResult;
       
    hResult = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&_mainText);
    if (FAILED(hResult))
        return hResult;

    hResult = _dev->CreateRenderTargetView(_mainText.Get(), NULL, &_mainRTV);
    if (FAILED(hResult))
        return hResult;

    D3D11_VIEWPORT vp;
    ZeroMemory(&vp, sizeof(D3D11_VIEWPORT));
    vp.Height = (FLOAT)height;
    vp.Width = (FLOAT)width;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _ctx->RSSetViewports(1, &vp);

    // disable z buffer
    D3D11_DEPTH_STENCIL_DESC stencilDesc;
    ZeroMemory(&stencilDesc, sizeof(stencilDesc));
    CComPtr<ID3D11DepthStencilState> pDepthStencilState;
    hResult = _dev->CreateDepthStencilState(&stencilDesc, &pDepthStencilState);
    if (SUCCEEDED(hResult)) {
        _ctx->OMSetDepthStencilState(pDepthStencilState, 0);
    }

    _hwnd = hwnd;

    return hResult;
}

#define HR_CHECK(X)         \
    do {                    \
        if (FAILED(X)) {   \
            __debugbreak(); \
        }                   \
    } while (0)



struct SimpleVertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 uv;
};

HRESULT d3d11render::_setupVertex()
{
    HRESULT hr = S_OK;

    SimpleVertex vertices[] = {
        { DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },   { DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) }, { DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) }, { DirectX::XMFLOAT3(-1.0f, 1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
    };

    ComPtr<ID3D11Buffer> vertexBuf = nullptr;

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(SimpleVertex) * ARRAYSIZE(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;

    hr = _dev->CreateBuffer(&bd, &InitData, &vertexBuf);
    if (SUCCEEDED(hr)) {
        _vertexBuf = vertexBuf;
    }

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    ComPtr<ID3D11SamplerState> sampler;
    hr = _dev->CreateSamplerState(&sampDesc, sampler.GetAddressOf());
    if (SUCCEEDED(hr))
        _sampler = sampler;

    return hr;
}

HRESULT d3d11render::_setupShader(VideoFrameType fmt)
{
    HRESULT hr = S_OK;
    ComPtr<ID3DBlob> vsBlob = nullptr;

    switch (fmt) {
        case VideoFrameType::kVideoFrameTypeI420:
        case VideoFrameType::kVideoFrameTypeNV12:
        {
        } break;
        case VideoFrameType::kVideoFrameTypeRGB24:
        case VideoFrameType::kVideoFrameTypeRGBA:
        case VideoFrameType::kVideoFrameTypeBGRA:
        {
            hr = CPGPU::CompileVertexShader(L"hlsl/vsRGBA.hlsl", "main", _dev.Get(), &vsBlob, nullptr);
            HR_CHECK(hr);

            ComPtr<ID3D11VertexShader> vs;
            hr = _dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr,
                                          vs.GetAddressOf());
            HR_CHECK(hr);
            _vs = vs;

            ComPtr<ID3DBlob> psBlob = nullptr;
            hr = CPGPU::CompilePixelShader(L"hlsl/psRGBA.hlsl", "main", _dev.Get(), &psBlob, nullptr);
            HR_CHECK(hr);

            ComPtr<ID3D11PixelShader> ps;
            hr = _dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr,
                                         ps.GetAddressOf());
            HR_CHECK(hr);
            _ps = ps;


        } break;
    }

    const D3D11_INPUT_ELEMENT_DESC layoutdesc[]
        = { { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ComPtr<ID3D11InputLayout> layout = NULL;
    hr = _dev->CreateInputLayout(layoutdesc, ARRAYSIZE(layoutdesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                 &layout);
    if (SUCCEEDED(hr)) {
        _layout = layout;
    }

    return hr;
}


HRESULT d3d11render::display(const VideoFrame &frame)
{
    HRESULT hRet = S_OK;

    auto fmtType = frame.type();
    CSize frameSize(frame.width(), frame.height());

    auto curNono = std::chrono::high_resolution_clock::now();

    if (_curPixType != fmtType || (frameSize != _curBuffSize)) {
        _setupVertex();

        _reallocTexture(frameSize, (int)frame.type());
        _curPixType = fmtType;
        _curBuffSize = frameSize;

        _setupShader(fmtType);
    }
    
    _updateTexture(frame);

    if (_hwndSz != _hwndSettingSz) {
        _mainRTV.Reset();
        _mainText.Reset();
        auto hr = _swapChain->ResizeBuffers(2, static_cast<uint32_t>(_hwndSettingSz.cx),
                                        static_cast<uint32_t>(_hwndSettingSz.cy),
                                            DXGI_FORMAT_R8G8B8A8_UNORM, 0);

        hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&_mainText);
        if (FAILED(hr))
            return hr;

        hr = _dev->CreateRenderTargetView(_mainText.Get(), NULL, &_mainRTV);
        if (FAILED(hr))
            return hr;

        _hwndSz = _hwndSettingSz;
    }

    {
        float vp_width = 0;
        float vp_height = 0;
        float vp_top = 0;
        float vp_left = 0;

        float frameRatio = frameSize.cx *1.0f / frameSize.cy;
        float wndRatio = _hwndSz.cx * 1.0f / _hwndSz.cy;

        if (wndRatio > frameRatio) {
            vp_width = _hwndSz.cy * frameRatio;
            vp_height = _hwndSz.cy;
            vp_left = (_hwndSz.cx - vp_width) / 2;
            vp_top = 0;
        }
        else {
            vp_width = _hwndSz.cx;
            vp_height = _hwndSz.cx / frameRatio;
            vp_left = 0;
            vp_top = (_hwndSz.cy - vp_height) / 2;
        }
        D3D11_VIEWPORT vp;
        ZeroMemory(&vp, sizeof(D3D11_VIEWPORT));
        vp.Height = vp_height;
        vp.Width = vp_width;
        vp.TopLeftX = vp_left;
        vp.TopLeftY = vp_top;
        _ctx->RSSetViewports(1, &vp);
    }

    constexpr float ClearColor[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
    _ctx->ClearRenderTargetView(_mainRTV.Get(), ClearColor);

    constexpr UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;

    _ctx->IASetVertexBuffers(0, 1, _vertexBuf.GetAddressOf(), &stride, &offset);
    _ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _ctx->IASetInputLayout(_layout.Get());

    _ctx->VSSetShader(_vs.Get(), nullptr, 0);
    _ctx->PSSetShader(_ps.Get(), nullptr, 0);

    _ctx->PSSetShaderResources(0, 1, _ySRV.GetAddressOf());
    _ctx->PSSetSamplers(0, 1, _sampler.GetAddressOf());

    _ctx->OMSetRenderTargets(1, _mainRTV.GetAddressOf(), NULL);

    _ctx->Draw(6, 0);

    hRet = _swapChain->Present(0, 0);

    return hRet;
}

HRESULT d3d11render::release()
{
    HRESULT ret = E_FAIL;

    if (_ctx)
        _ctx->ClearState();

    _yText.Reset();
    _uText.Reset();
    _vText.Reset();

    _ySRV.Reset();
    _uSRV.Reset();
    _vSRV.Reset();

    _swapChain.Reset();
    _mainRTV.Reset();
    _ctx.Reset();
    _dev.Reset();

    return ret;
}


HRESULT d3d11render::_reallocTexture(CSize destBufSize, int dformat)
{
    VideoFrameType destBufFormat = (VideoFrameType)dformat;
    HRESULT hRet = S_OK;
    auto &width = destBufSize.cx;
    auto &height = destBufSize.cy;

    _yText.Reset();
    _uText.Reset();
    _vText.Reset();

    _ySRV.Reset();
    _uSRV.Reset();
    _vSRV.Reset();

    switch (destBufFormat) {
    case VideoFrameType::kVideoFrameTypeI420:
    {
        if (SUCCEEDED(hRet)) {
            hRet = CPGPU::MakeTexAndSRV(_dev.Get(), width, height, DXGI_FORMAT_R8_UNORM, &_yText, &_ySRV,
                                          D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE);
        }

        if (SUCCEEDED(hRet)) {
            hRet = CPGPU::MakeTexAndSRV(_dev.Get(), width / 2, height / 2, DXGI_FORMAT_R8_UNORM, &_uText, &_uSRV,
                                          D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE);
        }

        if (SUCCEEDED(hRet)) {
            hRet = CPGPU::MakeTexAndSRV(_dev.Get(), width / 2, height / 2, DXGI_FORMAT_R8_UNORM, &_vText, &_vSRV,
                                          D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE);
        }
    } break;
    case VideoFrameType::kVideoFrameTypeNV12:
    {
        hRet = CPGPU::MakeTexAndSRV(_dev.Get(), width, height, DXGI_FORMAT_R8_UNORM, &_yText, &_ySRV,
                                      D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE);

        if (SUCCEEDED(hRet)) {
            hRet = CPGPU::MakeTexAndSRV(_dev.Get(), width, height, DXGI_FORMAT_R8_UNORM, &_uText, &_uSRV,
                                          D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE);
        }
    } break;
    case VideoFrameType::kVideoFrameTypeRGBA:
    case VideoFrameType::kVideoFrameTypeBGRA:
    case VideoFrameType::kVideoFrameTypeRGB24:
    {
        hRet = CPGPU::MakeTexAndSRV(_dev.Get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, &_yText, &_ySRV,
                                      D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE);
    } break;
    default:
        break;
    }

    return hRet;
}

static void fast_unpack(char *rgba, const char *rgb, const int count)
{
    if (count == 0)
        return;
    for (int i = count; --i; rgba += 4, rgb += 3) {
        *(uint32_t *)(void *)rgba = *(const uint32_t *)(const void *)rgb;
        std::swap(rgba[0], rgba[2]);
    }
    for (int j = 0; j < 3; ++j) {
        rgba[j] = rgb[j];
    }
}

HRESULT d3d11render::onResize(int cx, int cy)
{
    _hwndSettingSz = { cx, cy };
    return S_OK;
}

HRESULT d3d11render::_updateTexture(const VideoFrame &frame)
{
    HRESULT hRet = S_OK;

    auto fmt = frame.type();
    switch (fmt) {
    case VideoFrameType::kVideoFrameTypeI420:
    {
        D3D11_MAPPED_SUBRESOURCE map;
        ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
        hRet = _ctx->Map(_yText.Get(), 0, D3D11_MAP_WRITE, 0, &map);
        if (SUCCEEDED(hRet)) {
            uint8_t *pDest = (uint8_t *)map.pData;
            uint8_t *pSrc = (uint8_t *)frame.data();
            int stride = frame.stride();
            auto height = frame.height();
            for (auto i = 0; i < height; i++) {
                memcpy(pDest, pSrc, stride);
                pDest += map.RowPitch;
                pSrc += stride;
            }
        }
        _ctx->Unmap(_yText.Get(), 0);

        ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
        hRet = _ctx->Map(_uText.Get(), 0, D3D11_MAP_WRITE, 0, &map);
        if (SUCCEEDED(hRet)) {
            uint8_t *pDest = (uint8_t *)map.pData;
            uint8_t *pSrc = (uint8_t *)frame.data();
            int stride = frame.stride();
            auto height = frame.height() / 2;
            for (auto i = 0; i < height; i++) {
                memcpy(pDest, pSrc, stride);
                pDest += map.RowPitch;
                pSrc += stride;
            }
        }
        _ctx->Unmap(_uText.Get(), 0);

        ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
        hRet = _ctx->Map(_vText.Get(), 0, D3D11_MAP_WRITE, 0, &map);
        if (SUCCEEDED(hRet)) {
            uint8_t *pDest = (uint8_t *)map.pData;
            uint8_t *pSrc = (uint8_t *)frame.data();
            int stride = frame.stride();
            auto height = frame.height() / 2;
            for (auto i = 0; i < height; i++) {
                memcpy(pDest, pSrc, stride);
                pDest += map.RowPitch;
                pSrc += stride;
            }
        }
        _ctx->Unmap(_vText.Get(), 0);
    } break;

    case VideoFrameType::kVideoFrameTypeNV12:
    {
        D3D11_MAPPED_SUBRESOURCE map;
        ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
        hRet = _ctx->Map(_yText.Get(), 0, D3D11_MAP_WRITE, 0, &map);
        if (SUCCEEDED(hRet)) {
            uint8_t *pDest = (uint8_t *)map.pData;
            uint8_t *pSrc = (uint8_t *)frame.data();
            int stride = frame.stride();
            auto height = frame.height();
            for (auto i = 0; i < height; i++) {
                memcpy(pDest, pSrc, stride);
                pDest += map.RowPitch;
                pSrc += stride;
            }
        }
        _ctx->Unmap(_yText.Get(), 0);

        ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
        hRet = _ctx->Map(_uText.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
        if (SUCCEEDED(hRet)) {
            uint8_t *pDest = (uint8_t *)map.pData;
            uint8_t *pSrc = (uint8_t *)frame.data();
            int stride = frame.stride();
            auto height = frame.height();
            for (auto i = 0; i < height; i++) {
                memcpy(pDest, pSrc, stride);
                pDest += map.RowPitch;
                pSrc += stride;
            }
        }
        _ctx->Unmap(_uText.Get(), 0);
    } break;
    case VideoFrameType::kVideoFrameTypeRGB24:
    {
        D3D11_MAPPED_SUBRESOURCE map;
        ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
        hRet = _ctx->Map(_yText.Get(), 0, D3D11_MAP_WRITE, 0, &map);
        if (SUCCEEDED(hRet)) {
            uint8_t *pDest = (uint8_t *)map.pData;
            int stride = frame.stride();

            uint8_t *pSrc = (uint8_t *)frame.data();
            auto height = frame.height();
            for (auto i = 0; i < height; i++) {
                fast_unpack((char *)pDest, (char *)pSrc, frame.width());
                pDest += map.RowPitch;
                pSrc += stride;
            }
        }
        _ctx->Unmap(_yText.Get(), 0);
    } break;
    case VideoFrameType::kVideoFrameTypeRGBA:
    {
        D3D11_MAPPED_SUBRESOURCE map;
        ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
        hRet = _ctx->Map(_yText.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
        if (SUCCEEDED(hRet)) {
            uint8_t *pDest = (uint8_t *)map.pData;
            int stride = frame.stride();

            if (stride == map.RowPitch) {
                uint8_t *pSrc = (uint8_t *)frame.data();
                memcpy(pDest, pSrc, stride * frame.height());
            }
            else {
                uint8_t *pSrc = (uint8_t *)frame.data();
                auto height = frame.height();
                for (auto i = 0; i < height; i++) {
                    memcpy(pDest, pSrc, stride);
                    pDest += map.RowPitch;
                    pSrc += stride;
                }
            }
        }
        _ctx->Unmap(_yText.Get(), 0);
    } break;
    default:
        break;
    }

    return hRet;
}