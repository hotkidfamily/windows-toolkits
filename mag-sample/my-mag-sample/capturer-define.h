#pragma once

#include <Windows.h>
#include "VideoFrame.h"
#include "DesktopRect.h"

#include <vector>

typedef void (*funcCaptureCallback)(VideoFrame *frame, void *args);


class CCapture 
{
  public:
    CCapture(){};
    virtual ~CCapture(){};

  public:
    virtual bool startCaptureWindow(HWND hWnd) = 0;
    virtual bool startCaptureScreen(HMONITOR hMonitor) = 0;
    virtual bool stop() = 0;
    virtual bool captureImage(const DesktopRect &rect) = 0;
    virtual bool setCallback(funcCaptureCallback, void *) = 0;
    virtual bool setExcludeWindows(std::vector<HWND>& hWnd) = 0;
    virtual const char *getName() = 0;
    virtual bool usingTimer() = 0;
};