#pragma once
#include "capturer-define.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <memory>
#include "VideoFrame.h"

using namespace Microsoft::WRL;

class DXGICapture : public CCapture {
  public:
    DXGICapture();
    ~DXGICapture();

  public:
    bool startCaptureWindow(HWND hWnd) final;
    bool startCaptureScreen(HMONITOR hMonitor) final;
    bool stop() final;
    const char *getName() final;

  protected:
    void onCaptureLoop() final;

  private:
    bool _init(HMONITOR &hm);
    void _deinit();
    bool _loadD3D11();
    bool _captureImage(const DesktopRect &rect);
    bool _onCaptured(DXGI_MAPPED_RECT &rect, DXGI_OUTPUT_DESC &desc);

  private:
    ComPtr<ID3D11Device> _dev = nullptr;
    ComPtr<ID3D11DeviceContext> _ctx = nullptr;

    ComPtr<IDXGIOutputDuplication> _desktopDuplication = nullptr;
    DXGI_OUTPUT_DESC _outputDesc = {};

    D3D11_TEXTURE2D_DESC _sourceFormat;
    ComPtr<ID3D11Texture2D> _destFrame;

    std::unique_ptr<VideoFrame> _frames;
    DesktopRect _lastRect = {};

    decltype (::D3D11CreateDevice)* fnD3D11CreateDevice;
    decltype (::CreateDXGIFactory1)* fnCreateDXGIFactory1;
};
