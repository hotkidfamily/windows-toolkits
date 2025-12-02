#pragma once
#include "capturer-define.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <memory>
#include <mutex>
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
    bool captureImage(const DesktopRect &rect) final;
    bool setCallback(funcCaptureCallback, void *) final;
    bool setExcludeWindows(std::vector<HWND>& hWnd) final;
    const char *getName() final;
    bool usingTimer() final;
    
  public:
    bool onCaptured(DXGI_MAPPED_RECT &rect, DXGI_OUTPUT_DESC &desc);

  private:
    bool _init(HMONITOR &hm);
    void _deinit();
    bool _loadD3D11();

  private:
    ComPtr<ID3D11Device> _dev = nullptr;
    ComPtr<ID3D11DeviceContext> _ctx = nullptr;

    ComPtr<IDXGIOutputDuplication> _desktopDuplication = nullptr;
    DXGI_OUTPUT_DESC _outputDesc = {};

    D3D11_TEXTURE2D_DESC _sourceFormat;
    ComPtr<ID3D11Texture2D> _destFrame;

    std::unique_ptr<VideoFrame> _frames;
    DesktopRect _lastRect = {};

    std::recursive_mutex _cbMutex;
    funcCaptureCallback _callback = nullptr;
    void *_callbackargs = nullptr;

    decltype (::D3D11CreateDevice)* fnD3D11CreateDevice;
    decltype (::CreateDXGIFactory1)* fnCreateDXGIFactory1;

    std::vector<HWND> _coverdWindows;
};
