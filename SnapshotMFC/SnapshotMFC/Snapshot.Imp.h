#pragma once

#include <string>
#include <stdio.h>
#include <stdlib.h>

namespace Snapshot
{

struct snap_img_desc
{
    int width = 0;
    int rowPitch = 0;
    int height = 0;
    int channels = 0;
    uint8_t *data = nullptr;
    int data_len = 0;
};

namespace ImpGDI
{

int CaptureAnImageGDIFromDesktop(HWND hWnd, int reqWidth, int reqHeight, snap_img_desc &dest);
int CaptureAnImageGDI(HWND hWnd, int reqWidth, int reqHeight, snap_img_desc &dest);

int CaptureAnImageDWM(HWND hWnd, int reqWidth, int reqHeight, snap_img_desc &dest);

int CaptureMonitorByGDI(HMONITOR hMonitor, int reqWidth, int reqHeight, snap_img_desc &dest);

int FreeImage(snap_img_desc &desc);
} // namespace ImpGDI
} // namespace Snapshot
