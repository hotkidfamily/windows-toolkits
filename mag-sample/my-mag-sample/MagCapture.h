#pragma once

#include "capturer-define.h"

#include <memory>
#include <mutex>
#include <vector>

#include "MagFuncDefine.h"

class MagCapture : public CCapture
{
public:
    MagCapture();
    ~MagCapture();

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
    bool onCaptured(void *srcdata, MAGIMAGEHEADER srcheader);

protected:
    bool _initMagnifier(DesktopRect &rect);
    bool _destoryMagnifier();
    bool _loadMagnificationAPI();

    static BOOL WINAPI _OnMagImageScalingCallback(HWND hwnd,
                                                 void *srcdata,
                                                 MAGIMAGEHEADER srcheader,
                                                 void *destdata,
                                                 MAGIMAGEHEADER destheader,
                                                 RECT unclipped,
                                                 RECT clipped,
                                                 HRGN dirty);
private:
    std::unique_ptr<MagInterface> _api = nullptr;
    std::unique_ptr<VideoFrame> _frames;
    DesktopRect _lastRect;
    int32_t _offset = -1;

    HMODULE _hMagModule = nullptr;
    HINSTANCE _hMagInstance = nullptr;

    HWND _hostWnd = nullptr;
    HWND _magWnd = nullptr;

    std::recursive_mutex _cbMutex;
    funcCaptureCallback _callback = nullptr;
    void *_callbackargs = nullptr;
};

