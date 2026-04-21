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
    bool setExcludeWindows(std::vector<HWND>& hWnd) final;
    const char *getName() final;

  protected:
    void onCaptureLoop() final;

  private:
    bool _captureImage(const DesktopRect &rect);
    bool _onCaptured(void *srcdata, BITMAPINFOHEADER& srcheader);

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

    std::vector<HWND> _coverdWindows;
    std::recursive_mutex _excludeMutex;
    HWND _previousHwnd = nullptr;
};
