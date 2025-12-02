#include "CPGPU.h"

#include <array>
#include <memory>

#include <wrl/client.h>
#include <cassert>

using namespace Microsoft::WRL;



CPGPU::CPGPU()
{
    auto p = GetModuleHandle(L"d3d11.dll");
    if(!p)
        p = LoadLibrary(L"d3d11.dll");

    if(p)
        _d3d11CreateDevice = (decltype(_d3d11CreateDevice)) GetProcAddress(p, "D3D11CreateDevice");
}

CPGPU::~CPGPU()
{
    destoryDevice();
}

HRESULT CPGPU::_initDev()
{
    HRESULT hr = S_OK;

    // Driver types supported
    std::array<D3D_DRIVER_TYPE, 1> DriverTypes = { D3D_DRIVER_TYPE_HARDWARE };

    std::array<D3D_FEATURE_LEVEL, 3> FeatureLevels
        = { D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };

    D3D_FEATURE_LEVEL FeatureLevel;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> deviceCtx;

    // Create device
    for (UINT didx = 0; didx < DriverTypes.size(); ++didx)
    {
        UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        hr = _d3d11CreateDevice(nullptr, DriverTypes.at(didx), nullptr, createDeviceFlags, FeatureLevels.data(),
            (UINT)FeatureLevels.size(), D3D11_SDK_VERSION, &device, &FeatureLevel, &deviceCtx);
        if (SUCCEEDED(hr))
        {
            // Device creation succeeded, no need to loop anymore
            break;
        }
    }
    if (!device)
    {
        return ERROR_OPEN_FAILED;
    }

    _device = device;
    _deviceCtx = deviceCtx;

    return hr;
}

HRESULT CPGPU::createDevice()
{
    HRESULT hr = S_OK;

    hr = _initDev();

    return hr;
}

ID3D11Device* CPGPU::getDevice() const
{
    auto v = _device.Get();
    return v;
}

ID3D11DeviceContext* CPGPU::getContext() const
{
    auto v = _deviceCtx.Get();
    return v;
}

HRESULT CPGPU::destoryDevice()
{
    HRESULT ret = S_OK;

    if (_deviceCtx)
    {
        _deviceCtx->ClearState();
        _deviceCtx->Flush();
    }

    _deviceCtx.Reset();

    return ret;
}

int CPGPU::upload(ID3D11Texture2D* dst, const GraphFrame* src)
{
    assert(dst);
    assert(src);

    D3D11_MAPPED_SUBRESOURCE res{ 0 };
    auto hr = _deviceCtx->Map(dst, 0, D3D11_MAP_WRITE, 0, &res);
    if (SUCCEEDED(hr))
    {
        uint8_t* dstptr = reinterpret_cast<uint8_t*>(res.pData);
        auto pitch = res.RowPitch;

        copy_plane(dstptr, pitch, src->cpumem.data, src->stride, src->h);

        _deviceCtx->Unmap(dst, 0);
    }
    else
    {
        return -1;
    }

    return 0;
}


int CPGPU::download(GraphFrame* src)
{
    auto& text = src->gpumem.text;
    D3D11_TEXTURE2D_DESC desc{ 0 };
    text->GetDesc(&desc);
    ComPtr<ID3D11Texture2D> stage;
    if (src->gpumem.textStage)
    {
        stage = src->gpumem.textStage;
    }
    else
    {
        auto hr = CPGPU::MakeTex(_device.Get(), desc.Width, desc.Height, desc.Format, &stage, D3D11_USAGE_STAGING, 0,
            D3D11_CPU_ACCESS_READ);
        if (FAILED(hr))
        {
            return -3;
        }
        copy(stage.Get(), text.Get());
    }

    D3D11_MAPPED_SUBRESOURCE res{ 0 };
    auto hr = _deviceCtx->Map(stage.Get(), 0, D3D11_MAP_READ, 0, &res);
    if (SUCCEEDED(hr))
    {
        if (!src->cpumem.data)
        {
            return -1;
        }

        uint8_t* dstptr = src->cpumem.data;
        auto pitch = src->stride;

        uint8_t* srcptr = reinterpret_cast<uint8_t*>(res.pData);
        auto srcpitch = res.RowPitch;

        copy_plane(dstptr, pitch, srcptr, srcpitch, src->h);

        _deviceCtx->Unmap(stage.Get(), 0);
    }

    return 0;
}

void CPGPU::copy(ID3D11Resource* dst, ID3D11Resource* src)
{
    _deviceCtx->CopyResource(dst, src);
    //_deviceCtx->Flush();
}

void CPGPU::flush()
{
    _deviceCtx->Flush();
}

int CPGPU::runCompute(CPGPU* dev, FilterCtx* ctx)
{
    auto devCtx = dev->getContext();

    if (ctx->szCSs)
        devCtx->CSSetShader(ctx->CSs[0], nullptr, 0);

    if (ctx->szCBs)
        devCtx->CSSetConstantBuffers(0, ctx->szCBs, ctx->CBs);

    if (ctx->szSRVs)
        devCtx->CSSetShaderResources(0, ctx->szSRVs, ctx->SRVs);

    if (ctx->szUAVs)
        devCtx->CSSetUnorderedAccessViews(0, ctx->szUAVs, ctx->UAVs, nullptr);

    devCtx->Dispatch(ctx->threads.x, ctx->threads.y, 1);

    {
        // clear up
        devCtx->CSSetShader(nullptr, nullptr, 0);
        ID3D11Buffer* clearCBs[4] = { nullptr };
        devCtx->CSSetConstantBuffers(0, 4, clearCBs);
        ID3D11UnorderedAccessView* clearUAVs[4] = { nullptr };
        devCtx->CSSetUnorderedAccessViews(0, 4, clearUAVs, nullptr);
        ID3D11ShaderResourceView* clearSRVs[4] = { nullptr };
        devCtx->CSSetShaderResources(0, 4, clearSRVs);
    }
    devCtx->Flush();
    return 0;
}

bool CPGPU::isSupport()
{
    auto dev = _device;

    if (dev->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0)
    {
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts = { 0 };
        (void)dev->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts));
        if (!hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
        {
            return false;
        }
    }
    return true;
}


HRESULT CPGPU::Make1DTex(ID3D11Device* device,
    int w,
    DXGI_FORMAT fmt,
    ID3D11Texture1D** text,
    D3D11_USAGE Usage,
    UINT BindFlags,
    UINT CPUAccessFlags)
{
    D3D11_TEXTURE1D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE1D_DESC));
    desc.Width = w;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = fmt;
    desc.Usage = Usage;
    desc.BindFlags = BindFlags;
    desc.CPUAccessFlags = CPUAccessFlags;

    HRESULT hr = device->CreateTexture1D(&desc, NULL, text);
    return hr;
}

HRESULT CPGPU::Make1DTexAndUAV(ID3D11Device* device,
    int w,
    DXGI_FORMAT fmt,
    ID3D11Texture1D** text,
    ID3D11UnorderedAccessView** view,
    D3D11_USAGE usage,
    UINT bindflags,
    UINT cpuflags)
{
    HRESULT hr = Make1DTex(device, w, fmt, text, usage, bindflags, cpuflags);
    if (SUCCEEDED(hr))
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        ZeroMemory(&uavDesc, sizeof(uavDesc));
        uavDesc.Format = fmt;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
        uavDesc.Texture1D.MipSlice = 0;
        hr = device->CreateUnorderedAccessView(*text, &uavDesc, view);
    }

    return hr;
}

HRESULT CPGPU::Make1DSRV(ID3D11Device* device, ID3D11Texture1D* text, ID3D11ShaderResourceView** res)
{
    HRESULT hr = S_OK;
    D3D11_TEXTURE1D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE1D_DESC));
    text->GetDesc(&desc);

    D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
    ZeroMemory(&resourceDesc, sizeof(resourceDesc));
    resourceDesc.Format = desc.Format;
    resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
    resourceDesc.Texture1D.MipLevels = desc.MipLevels;
    resourceDesc.Texture1D.MostDetailedMip = 0;
    hr = device->CreateShaderResourceView(text, &resourceDesc, res);

    return hr;
}

HRESULT CPGPU::MakeTex(ID3D11Device* device,
    int w,
    int h,
    DXGI_FORMAT fmt,
    ID3D11Texture2D** text,
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

HRESULT CPGPU::MakeTexAndUAV(ID3D11Device* device,
    int w,
    int h,
    DXGI_FORMAT fmt,
    ID3D11Texture2D** text,
    ID3D11UnorderedAccessView** view,
    D3D11_USAGE usage,
    UINT bindflags,
    UINT cpuflags)
{
    HRESULT hr = MakeTex(device, w, h, fmt, text, usage, bindflags, cpuflags);
    if (SUCCEEDED(hr))
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        ZeroMemory(&uavDesc, sizeof(uavDesc));
        uavDesc.Format = fmt;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        hr = device->CreateUnorderedAccessView(*text, &uavDesc, view);
    }

    return hr;
}

HRESULT CPGPU::MakeUAV(ID3D11Device* device, ID3D11Texture2D* text, ID3D11UnorderedAccessView** view)
{
    HRESULT hr = S_OK;
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
    text->GetDesc(&desc);
    if (SUCCEEDED(hr))
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        ZeroMemory(&uavDesc, sizeof(uavDesc));
        uavDesc.Format = desc.Format;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        hr = device->CreateUnorderedAccessView(text, &uavDesc, view);
    }

    return hr;
}

HRESULT CPGPU::MakeTexAndSRV(ID3D11Device* device,
    int w,
    int h,
    DXGI_FORMAT fmt,
    ID3D11Texture2D** text,
    ID3D11ShaderResourceView** res,
    D3D11_USAGE usage,
    UINT bindflags,
    UINT cpuflags)
{
    HRESULT hr = MakeTex(device, w, h, fmt, text, usage, bindflags, cpuflags);
    if (SUCCEEDED(hr))
    {
        hr = MakeSRV(device, *text, res);
    }
    return hr;
}

HRESULT CPGPU::MakeSRV(ID3D11Device* device, ID3D11Texture2D* text, ID3D11ShaderResourceView** res)
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

HRESULT CPGPU::MakeConstBuffer(ID3D11Device* device, uint32_t size, void* initvalue, ID3D11Buffer** res)
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