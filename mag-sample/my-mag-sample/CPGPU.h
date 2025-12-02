#pragma once

#include<d3d11.h>
#include<wrl/client.h>
#include<dxgi1_2.h>


namespace CPGPU {
HRESULT CompileVertexShader(LPCWSTR srcFile,
                              LPCSTR entryPoint,
                              ID3D11Device *device,
                              ID3DBlob **blob,
                              const D3D_SHADER_MACRO defines[] = nullptr);

HRESULT CompilePixelShader(LPCWSTR srcFile,
                           LPCSTR entryPoint,
                           ID3D11Device *device,
                           ID3DBlob **blob,
                           const D3D_SHADER_MACRO defines[] = nullptr);

HRESULT CompileComputeShader(LPCWSTR srcFile,
                             LPCSTR entryPoint,
                             ID3D11Device *device,
                             ID3DBlob **blob,
                             const D3D_SHADER_MACRO defines[] = nullptr);
auto CreateD3D11Device(D3D_DRIVER_TYPE const type, UINT flags, Microsoft::WRL::ComPtr<ID3D11Device> &device);
auto CreateD3D11Device(UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT);
auto CreateDXGISwapChain(Microsoft::WRL::ComPtr<ID3D11Device> const device,
                         uint32_t width,
                         uint32_t height,
                         DXGI_FORMAT format,
                         uint32_t bufferCount);
HRESULT MakeTex(ID3D11Device *device,
                       int w,
                       int h,
                       DXGI_FORMAT fmt,
                       ID3D11Texture2D **text,
                       D3D11_USAGE Usage,
                       UINT BindFlags,
                       UINT CPUAccessFlags);
HRESULT MakeTexAndUAV(ID3D11Device *device,
                             int w,
                             int h,
                             DXGI_FORMAT fmt,
                             ID3D11Texture2D **text,
                             ID3D11UnorderedAccessView **view,
                             D3D11_USAGE usage,
                             UINT bindflags,
                             UINT cpuflags);
HRESULT MakeUAV(ID3D11Device *device, ID3D11Texture2D *text, ID3D11UnorderedAccessView **view);
HRESULT MakeConstBuffer(ID3D11Device *device, uint32_t size, void *initvalue, ID3D11Buffer **res);
HRESULT MakeSRV(ID3D11Device *device, ID3D11Texture2D *text, ID3D11ShaderResourceView **res);
HRESULT MakeTexAndSRV(ID3D11Device *device,
                      int w,
                      int h,
                      DXGI_FORMAT fmt,
                      ID3D11Texture2D **text,
                      ID3D11ShaderResourceView **res,
                      D3D11_USAGE usage,
                      UINT bindflags,
                      UINT cpuflags);
};
