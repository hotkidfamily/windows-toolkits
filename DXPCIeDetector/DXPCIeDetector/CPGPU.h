#pragma once

#pragma warning(push)
#pragma warning(disable : 4005 4838)
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <wrl/client.h>
#pragma warning(pop)

#include <cstdint>

#include "vframe.h"
#include "iFilter.h"

class CPGPU {
public:
    CPGPU();
    ~CPGPU();

    int upload(ID3D11Texture2D* dst, const GraphFrame* src);
    int download(GraphFrame* src);
    void copy(ID3D11Resource* dst, ID3D11Resource* src);
    bool isSupport();
    void flush();

    HRESULT createDevice();
    HRESULT destoryDevice();

    ID3D11Device* getDevice() const;
    ID3D11DeviceContext* getContext() const;

public:
    static int runCompute(CPGPU* dev, FilterCtx* ctx);

    static HRESULT Make1DTexAndUAV(ID3D11Device* device,
        int w,
        DXGI_FORMAT fmt,
        ID3D11Texture1D** text,
        ID3D11UnorderedAccessView** view,
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
        UINT bindflags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
        UINT cpuflags = 0);

    static HRESULT Make1DTex(ID3D11Device* device,
        int w,
        DXGI_FORMAT fmt,
        ID3D11Texture1D** text,
        D3D11_USAGE Usage = D3D11_USAGE_DYNAMIC,
        UINT BindFlags = D3D11_BIND_SHADER_RESOURCE,
        UINT CPUAccessFlags = 0);
    static HRESULT Make1DSRV(ID3D11Device* device, ID3D11Texture1D* text, ID3D11ShaderResourceView** res);

    static HRESULT MakeTexAndUAV(ID3D11Device* device,
        int w,
        int h,
        DXGI_FORMAT fmt,
        ID3D11Texture2D** text,
        ID3D11UnorderedAccessView** view,
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
        UINT bindflags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
        UINT cpuflags = 0);

    static HRESULT MakeTex(ID3D11Device* device,
        int w,
        int h,
        DXGI_FORMAT fmt,
        ID3D11Texture2D** text,
        D3D11_USAGE Usage = D3D11_USAGE_DYNAMIC,
        UINT BindFlags = D3D11_BIND_SHADER_RESOURCE,
        UINT CPUAccessFlags = 0);

    static HRESULT MakeUAV(ID3D11Device* device, ID3D11Texture2D* text, ID3D11UnorderedAccessView** view);

    static HRESULT MakeTexAndSRV(ID3D11Device* device,
        int w,
        int h,
        DXGI_FORMAT fmt,
        ID3D11Texture2D** text,
        ID3D11ShaderResourceView** res,
        D3D11_USAGE Usage = D3D11_USAGE_DYNAMIC,
        UINT BindFlags = D3D11_BIND_SHADER_RESOURCE,
        UINT CPUAccessFlags = 0);

    static HRESULT MakeSRV(ID3D11Device* device, ID3D11Texture2D* text, ID3D11ShaderResourceView** res);

    static HRESULT MakeConstBuffer(ID3D11Device* device, uint32_t size, void* initvalue, ID3D11Buffer** res);

private:
    HRESULT _initDev();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> _device = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _deviceCtx = nullptr;
    decltype(::D3D11CreateDevice)* _d3d11CreateDevice = nullptr;
};
