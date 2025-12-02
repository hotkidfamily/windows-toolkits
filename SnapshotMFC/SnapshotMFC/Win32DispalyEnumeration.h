#pragma once

#include <Windows.h>
#include <vector>

struct Monitor
{
  public:
    Monitor(nullptr_t)
    {
    }
    Monitor(HMONITOR hm, HDC hDC, RECT &rect)
    {
        _hmonitor = hm;
        _hDC = hDC;
        _rect = rect;
    }

    HMONITOR hMonitor() const noexcept
    {
        return _hmonitor;
    }

    HDC hDC() const noexcept
    {
        return _hDC;
    }

    RECT rect() const noexcept
    {
        return _rect;
    }

  private:
    HMONITOR _hmonitor = nullptr;
    HDC _hDC = nullptr;
    RECT _rect = {0};
};

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    auto monitors = reinterpret_cast<std::vector<Monitor> *>(dwData);

    Monitor m(hMonitor, hdcMonitor, *lprcMonitor);
    monitors->emplace_back(m);

    return TRUE;
}

std::vector<Monitor> enumMonitor()
{
    std::vector<Monitor> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&monitors);
    return monitors;
}
