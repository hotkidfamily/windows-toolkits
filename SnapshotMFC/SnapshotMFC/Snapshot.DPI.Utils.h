#pragma once

#include <string>

namespace Snapshot
{
namespace DPI
{
int LoadAPI();
void SupportDPI();

int GuessSystemDpi(HWND);

int GetDpiForWindow(HWND);
int GetDpiForSystem();

UINT GetMonitorDpiByWindow(HWND);
UINT GetMonitorDXGIWidth(HMONITOR hm);

} // namespace DPI
} // namespace Snapshot
