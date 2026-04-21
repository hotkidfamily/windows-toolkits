#pragma once

#include "capturer-define.h"

#include "d3d11render.h"
#include "SlidingWindow.h"

#include <vector>

const uint32_t KDefaultFPS = 30;

typedef struct tagAppContext
{
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
