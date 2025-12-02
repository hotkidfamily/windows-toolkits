#pragma once

#include "capturer-define.h"
#include <mutex>

class GDICapture : public CCapture {
  public:
    GDICapture();
    ~GDICapture();

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
    bool onCaptured(void *srcdata, BITMAPINFOHEADER& srcheader);

  private:
    HWND _hwnd{ nullptr };
    HMONITOR _hmonitor{ nullptr };

    HDC _monitorDC = nullptr;
    HDC _compatibleDC = nullptr;
    HBITMAP _hDibBitmap = nullptr;
    void *_hdstPtr = nullptr;
    BITMAPINFO bmi;

    std::unique_ptr<VideoFrame> _frames;
    DesktopRect _lastRect;

    std::recursive_mutex _cbMutex;
    funcCaptureCallback _callback = nullptr;
    void *_callbackargs = nullptr;

    std::vector<HWND> _coverdWindows;
    HWND _previousHwnd = nullptr;
};
