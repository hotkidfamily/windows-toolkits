#pragma once

#include <d3d11.h>

#include "capturer-define.h"
#include <memory>
#include <mutex>
#include "VideoFrame.h"


class DWMCapture : public CCapture {
  public:
    DWMCapture();
    ~DWMCapture();

  public:
    bool startCaptureWindow(HWND hWnd) final;
    bool startCaptureScreen(HMONITOR hMonitor) final;
    bool stop() final;
    bool captureImage(const DesktopRect &rect) final;
    bool setCallback(funcCaptureCallback, void *) final;
    bool setExcludeWindows(std::vector<HWND>& hWnd) final;
    
    const char *getName() final { return "DWMCapture";}
    bool usingTimer() final { return false; }

    int CaptureAnImageGDI();

  private:
    bool _onCaptured(uint8_t* data, CRect &);
    void _run();
    bool _createSession();
    bool _stopSession();

  private:
    std::thread _thread;
    bool _exit = true;

    HWND _hostWnd{};

    HWND _hwnd = nullptr;
    HMONITOR _hmonitor = nullptr;
    HTHUMBNAIL _thumbnail = nullptr;


    std::unique_ptr<VideoFrame> _frames;
    DesktopRect _lastRect = {};

    std::recursive_mutex _cbMutex;
    funcCaptureCallback _callback = nullptr;
    void *_callbackargs = nullptr;


    enum class ACTION
    {
        ACTION_Idle, ACTION_Start, ACTION_Stop, ACTION_Busy,
    };

    ACTION _action = ACTION::ACTION_Idle;
};
