#pragma once

#include <Unknwn.h>
#include <winrt/base.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/windows.graphics.capture.h>
#include <windows.graphics.capture.interop.h>
#include <winrt/windows.graphics.directx.h>
#include <winrt/windows.graphics.directx.direct3d11.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <DispatcherQueue.h>
#include <d3d11.h>

#include "capturer-define.h"
#include <memory>
#include <mutex>
#include "VideoFrame.h"


class WGCCapture : public CCapture {
  public:
    WGCCapture();
    ~WGCCapture();

  public:
    bool startCaptureWindow(HWND hWnd) final;
    bool startCaptureScreen(HMONITOR hMonitor) final;
    bool stop() final;
    bool captureImage(const DesktopRect &rect) final;
    bool setCallback(funcCaptureCallback, void *) final;
    bool setExcludeWindows(std::vector<HWND>& hWnd) final;
    
    const char *getName() final { return "Windows.Graphics.Capture";}
    bool usingTimer() final { return false; }


  private:
    void _onFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const &sender,
                         winrt::Windows::Foundation::IInspectable const &);
    bool _onCaptured(D3D11_MAPPED_SUBRESOURCE &rect, D3D11_TEXTURE2D_DESC &header);
    void _run();
    bool _createSession();
    bool _stopSession();

  private:
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem _item{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool _framePool{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession _session{ nullptr };
    winrt::com_ptr<ID3D11Device> _d3d11device{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker _frame_arrived_revoker;
    winrt::com_ptr<ID3D11Texture2D> _tempTexture{ nullptr };
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice _d3dDevice{ nullptr };
    winrt::Windows::Graphics::SizeInt32 _curFramePoolSz{ 0 };

    winrt::com_ptr<winrt::Windows::System::IDispatcherQueueController> _queueController { nullptr };
    winrt::Windows::System::DispatcherQueue* _queue{ nullptr };

    std::thread _thread;
    bool _exit = true;
    DWORD _threadID = -1;

    HWND _hwnd = nullptr;
    HMONITOR _hmonitor = nullptr;

    std::unique_ptr<VideoFrame> _frames;
    DesktopRect _lastRect = {};

    std::recursive_mutex _cbMutex;
    funcCaptureCallback _callback = nullptr;
    void *_callbackargs = nullptr;
};
