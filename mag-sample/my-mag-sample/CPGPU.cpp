#include "stdafx.h"
#include "CPGPU.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace CPGPU
{
HRESULT CompileVertexShader(LPCWSTR srcFile,
                            LPCSTR entryPoint,
                            ID3D11Device *device,
                            ID3DBlob **blob,
                            const D3D_SHADER_MACRO defines[])
{
    if (!srcFile || !entryPoint || !device || !blob)
        return E_INVALIDARG;

    *blob = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // We generally prefer to use the higher vs shader profile when possible
    // as vs 5.0 is better performance on 11-class hardware
    LPCSTR profile = (device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "vs_5_0" : "vs_4_0";

    ID3DBlob *shaderBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(srcFile, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, profile, flags, 0,
                                    &shaderBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char *)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }

        if (shaderBlob)
            shaderBlob->Release();

        return hr;
    }

    *blob = shaderBlob;

    return hr;
}

HRESULT CompilePixelShader(LPCWSTR srcFile,
                           LPCSTR entryPoint,
                           ID3D11Device *device,
                           ID3DBlob **blob,
                           const D3D_SHADER_MACRO defines[])
{
    if (!srcFile || !entryPoint || !device || !blob)
        return E_INVALIDARG;

    *blob = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // We generally prefer to use the higher CS shader profile when possible
    // as ps 5.0 is better performance on 11-class hardware
    LPCSTR profile = (device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "ps_5_0" : "ps_4_0";

    ID3DBlob *shaderBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(srcFile, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, profile, flags, 0,
                                    &shaderBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char *)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }

        if (shaderBlob)
            shaderBlob->Release();

        return hr;
    }

    *blob = shaderBlob;

    return hr;
}

HRESULT CompileComputeShader(LPCWSTR srcFile,
                             LPCSTR entryPoint,
                             ID3D11Device *device,
                             ID3DBlob **blob,
                             const D3D_SHADER_MACRO defines[])
{
    if (!srcFile || !entryPoint || !device || !blob)
        return E_INVALIDARG;

    *blob = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // We generally prefer to use the higher CS shader profile when possible
    // as CS 5.0 is better performance on 11-class hardware
    LPCSTR profile = (device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";

    ID3DBlob *shaderBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(srcFile, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, profile, flags, 0,
                                    &shaderBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char *)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }

        if (shaderBlob)
            shaderBlob->Release();

        return hr;
    }

    *blob = shaderBlob;

    return hr;
}

HRESULT MakeTex(ID3D11Device *device,
                int w,
                int h,
                DXGI_FORMAT fmt,
                ID3D11Texture2D **text,
                D3D11_USAGE Usage,
                UINT BindFlags,
                UINT CPUAccessFlags)
{
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = fmt;
    desc.SampleDesc = { 1, 0 };
    desc.Usage = Usage;
    desc.BindFlags = BindFlags;
    desc.CPUAccessFlags = CPUAccessFlags;

    HRESULT hr = device->CreateTexture2D(&desc, NULL, text);
    return hr;
}

HRESULT MakeTexAndUAV(ID3D11Device *device,
                      int w,
                      int h,
                      DXGI_FORMAT fmt,
                      ID3D11Texture2D **text,
                      ID3D11UnorderedAccessView **view,
                      D3D11_USAGE usage,
                      UINT bindflags,
                      UINT cpuflags)
{
    HRESULT hr = MakeTex(device, w, h, fmt, text, usage, bindflags, cpuflags);
    if (SUCCEEDED(hr)) {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        ZeroMemory(&uavDesc, sizeof(uavDesc));
        uavDesc.Format = fmt;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        hr = device->CreateUnorderedAccessView(*text, &uavDesc, view);
    }

    return hr;
}

HRESULT MakeUAV(ID3D11Device *device, ID3D11Texture2D *text, ID3D11UnorderedAccessView **view)
{
    HRESULT hr = S_OK;
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
    text->GetDesc(&desc);
    if (SUCCEEDED(hr)) {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        ZeroMemory(&uavDesc, sizeof(uavDesc));
        uavDesc.Format = desc.Format;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        hr = device->CreateUnorderedAccessView(text, &uavDesc, view);
    }

    return hr;
}

HRESULT MakeTexAndSRV(ID3D11Device *device,
                      int w,
                      int h,
                      DXGI_FORMAT fmt,
                      ID3D11Texture2D **text,
                      ID3D11ShaderResourceView **res,
                      D3D11_USAGE usage,
                      UINT bindflags,
                      UINT cpuflags)
{
    HRESULT hr = MakeTex(device, w, h, fmt, text, usage, bindflags, cpuflags);
    if (SUCCEEDED(hr)) {
        hr = MakeSRV(device, *text, res);
    }
    return hr;
}

HRESULT MakeSRV(ID3D11Device *device, ID3D11Texture2D *text, ID3D11ShaderResourceView **res)
{
    HRESULT hr = S_OK;
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
    text->GetDesc(&desc);

    D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
    ZeroMemory(&resourceDesc, sizeof(resourceDesc));
    resourceDesc.Texture2D.MostDetailedMip = 0;
    resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    resourceDesc.Texture2D.MipLevels = desc.MipLevels;
    resourceDesc.Format = desc.Format;
    hr = device->CreateShaderResourceView(text, &resourceDesc, res);

    return hr;
}

HRESULT MakeConstBuffer(ID3D11Device *device, uint32_t size, void *initvalue, ID3D11Buffer **res)
{
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = size;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = initvalue;

    auto hr = device->CreateBuffer(&bd, &InitData, res);
    return hr;
}

auto CreateD3D11Device(D3D_DRIVER_TYPE const type, UINT flags, Microsoft::WRL::ComPtr<ID3D11Device> &device)
{
    return D3D11CreateDevice(nullptr, type, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, device.GetAddressOf(),
                             nullptr, nullptr);
}

auto CreateD3D11Device(UINT flags)
{
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    HRESULT hr = CreateD3D11Device(D3D_DRIVER_TYPE_HARDWARE, flags, device);
    if (DXGI_ERROR_UNSUPPORTED == hr) {
        hr = CreateD3D11Device(D3D_DRIVER_TYPE_WARP, flags, device);
    }

    return device;
}

auto CreateDXGISwapChain(Microsoft::WRL::ComPtr<ID3D11Device> const device,
                         uint32_t width,
                         uint32_t height,
                         DXGI_FORMAT format,
                         uint32_t bufferCount)
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.Format = format;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferCount = bufferCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    Microsoft::WRL::ComPtr<IDXGIDevice2> dxgi;
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain;

    auto hr = device.As(&dxgi);
    if (FAILED(hr)) {
    }
    hr = dxgi->GetParent(_uuidof(IDXGIAdapter), (void **)&adapter);
    if (FAILED(hr)) {
    }
    hr = adapter->GetParent(_uuidof(IDXGIFactory2), (void **)&factory);
    if (FAILED(hr)) {
    }
    hr = factory->CreateSwapChainForComposition(device.Get(), &desc, nullptr, swapchain.GetAddressOf());
    if (FAILED(hr)) {
    }

    return swapchain;
}

} // namespace CPGPU