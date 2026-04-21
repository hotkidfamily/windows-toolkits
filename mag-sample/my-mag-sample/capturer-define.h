#pragma once

#include <Windows.h>
#include "VideoFrame.h"
#include "DesktopRect.h"

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

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
    virtual const char *getName() = 0;

    // Optional overrides
    virtual bool setExcludeWindows(std::vector<HWND>& hWnd) { return false; }

    void setCallback(funcCaptureCallback cb, void *args)
    {
        std::lock_guard<std::recursive_mutex> guard(_cbMutex);
        _callback = cb;
        _callbackArgs = args;
    }

    void updateRect(const DesktopRect &rect)
    {
        std::lock_guard<std::recursive_mutex> guard(_rectMutex);
        _captureRect = rect;
    }

  protected:
    // -- Callback delivery (shared by all backends) --
    void invokeCallback(VideoFrame *frame)
    {
        std::lock_guard<std::recursive_mutex> guard(_cbMutex);
        if (_callback)
            _callback(frame, _callbackArgs);
    }

    DesktopRect getCaptureRect()
    {
        std::lock_guard<std::recursive_mutex> guard(_rectMutex);
        return _captureRect;
    }

    // -- Default capture loop: sleep-based thread --
    // Subclasses that use a simple polling loop can call startLoop/stopLoop
    // and implement onCaptureLoop() to do one frame of work.
    virtual void onCaptureLoop() {}

    void startLoop(uint32_t fps = 30)
    {
        _loopRunning = true;
        _loopFps = fps;
        _loopThread = std::thread(&CCapture::_loopFunc, this);
    }

    void stopLoop()
    {
        _loopRunning = false;
        if (_loopThread.joinable())
            _loopThread.join();
    }

  private:
    void _loopFunc()
    {
        auto interval = std::chrono::milliseconds(1000 / _loopFps);
        while (_loopRunning) {
            auto t0 = std::chrono::high_resolution_clock::now();
            onCaptureLoop();
            auto elapsed = std::chrono::high_resolution_clock::now() - t0;
            auto remaining = interval - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            if (remaining.count() > 0)
                std::this_thread::sleep_for(remaining);
        }
    }

    std::recursive_mutex _cbMutex;
    funcCaptureCallback _callback = nullptr;
    void *_callbackArgs = nullptr;

    std::recursive_mutex _rectMutex;
    DesktopRect _captureRect;

    std::thread _loopThread;
    std::atomic<bool> _loopRunning{ false };
    uint32_t _loopFps = 30;
};
