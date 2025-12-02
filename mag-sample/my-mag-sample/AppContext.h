#pragma once

#include "capturer-define.h"

#include "d3d11render.h"
#include "SlidingWindow.h"

#include <vector>
#include <thread>

const uint32_t KDefaultFPS = 30;
const uint32_t KThreadCaptureMessage = WM_USER+1;

typedef struct tagAppContext
{
    struct {
        UINT timerID = 0;
        UINT_PTR timerInst = 0;
        uint32_t fps = KDefaultFPS;

        bool bRunning = false;
        std::thread capThread;
    }timer;

    struct {
        std::unique_ptr<CCapture> host;

        HWND winID = 0;
        HMONITOR screenID = 0;

        DesktopRect rect{};

        std::unique_ptr<utils::SlidingWindow> statics;
    }cpt;

    struct
    {
        std::unique_ptr<d3d11render> d3d11render;
        std::unique_ptr<utils::SlidingWindow> statics;
    }render;
}AppContext;