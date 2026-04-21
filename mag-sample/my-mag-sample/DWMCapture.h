#pragma once

#include <d3d11.h>

#include "capturer-define.h"
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include "VideoFrame.h"


class DWMCapture : public CCapture {
  public:
    DWMCapture();
    ~DWMCapture();

  public:
    bool startCaptureWindow(HWND hWnd) final;
    bool startCaptureScreen(HMONITOR hMonitor) final;
    bool stop() final;
    
    const char *getName() final { return "DWMCapture";}

    int CaptureAnImageGDI();

  private:
    bool _onCaptured(uint8_t* data, CRect &);
    void _run();
    bool _createSession();
    bool _stopSession();
    void _updateThumbnailSize();

  private:
    std::thread _thread;
    std::atomic<bool> _exit{ true };

    HWND _hostWnd{};

    HWND _hwnd = nullptr;
    HMONITOR _hmonitor = nullptr;
    HTHUMBNAIL _thumbnail = nullptr;
    SIZE _lastThumbSize{};

    std::unique_ptr<VideoFrame> _frames;
    DesktopRect _lastRect = {};

    enum class ACTION
    {
        ACTION_Idle, ACTION_Start, ACTION_Stop, ACTION_Busy,
    };

    ACTION _action = ACTION::ACTION_Idle;
    uint32_t _fps = 30;
};
