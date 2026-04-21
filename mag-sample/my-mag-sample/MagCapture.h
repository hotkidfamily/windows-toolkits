#pragma once

#include "capturer-define.h"

#include <memory>
#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>

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
    bool setExcludeWindows(std::vector<HWND>& hWnd) final;
    const char *getName() final;
    
  public:
    bool onCaptured(void *srcdata, MAGIMAGEHEADER srcheader);

protected:
    bool _initMagnifier(DesktopRect &rect);
    bool _destoryMagnifier();
    bool _loadMagnificationAPI();
    bool _captureImage(const DesktopRect &capRect);

    static BOOL WINAPI _OnMagImageScalingCallback(HWND hwnd,
                                                 void *srcdata,
                                                 MAGIMAGEHEADER srcheader,
                                                 void *destdata,
                                                 MAGIMAGEHEADER destheader,
                                                 RECT unclipped,
                                                 RECT clipped,
                                                 HRGN dirty);

    void _captureLoop();

private:
    bool _startCapture(DesktopRect initRect);

    std::unique_ptr<MagInterface> _api = nullptr;
    std::unique_ptr<VideoFrame> _frames;
    DesktopRect _lastRect;
    int32_t _offset = -1;

    HMODULE _hMagModule = nullptr;
    HINSTANCE _hMagInstance = nullptr;

    HWND _hostWnd = nullptr;
    HWND _magWnd = nullptr;

    // Own message-loop thread (Mag API requires message pump)
    std::thread _msgThread;
    std::atomic<bool> _running{ false };
    DWORD _msgThreadID = 0;
    uint32_t _fps = 30;

    // Synchronization for thread-side init
    DesktopRect _initRect;
    std::mutex _initMutex;
    std::condition_variable _initCV;
    bool _initDone = false;
    bool _initResult = false;
};
