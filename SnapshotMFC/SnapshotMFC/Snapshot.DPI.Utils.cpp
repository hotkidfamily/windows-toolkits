#include "pch.h"
#include "Snapshot.Utils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windef.h>
#include <shellscalingapi.h>
#include <wrl/client.h>
#include <dxgi.h>

#include <string>
#include "Snapshot.DPI.Utils.h"
#include "WinVersionHelper.h"

#pragma comment(lib, "Shcore.lib")

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
typedef enum DPI_AWARENESS_CONTEXT
{
    DPI_AWARENESS_CONTEXT_UNAWARE = -1,
    DPI_AWARENESS_CONTEXT_SYSTEM_AWARE = -2,
    DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE = -3,
    DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4
} DPI_AWARENESS_CONTEXT;
#endif
typedef DPI_AWARENESS_CONTEXT(WINAPI *SetThreadDpiAwarenessContextFunc)(
    DPI_AWARENESS_CONTEXT dpiContext);
typedef BOOL(WINAPI *SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT value);
typedef UINT(WINAPI *GetDpiForWindowFunc)(HWND hwnd);
typedef UINT(WINAPI *GetDpiForSystemFunc)();


namespace Snapshot
{

namespace DPI
{
static SetThreadDpiAwarenessContextFunc pSetThreadDpiAwarenessContext{ nullptr };
static SetProcessDpiAwarenessContextFunc pSetProcessDpiAwarenessContext = nullptr;
static GetDpiForWindowFunc pGetDpiForWindow = nullptr;
static GetDpiForSystemFunc pGetDpiForSystem = nullptr;

int LoadAPI()
{
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");
    if (!hUser32)
    {
        hUser32 = LoadLibrary(L"user32.dll");
        if (!hUser32)
        {
            return -1;
        }
    }

    pSetProcessDpiAwarenessContext
        = reinterpret_cast<SetProcessDpiAwarenessContextFunc>(
            GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));

    pSetThreadDpiAwarenessContext = reinterpret_cast<SetThreadDpiAwarenessContextFunc>(
        GetProcAddress(hUser32, "SetThreadDpiAwarenessContext"));

    pGetDpiForWindow
        = reinterpret_cast<GetDpiForWindowFunc>(GetProcAddress(hUser32, "GetDpiForWindow"));

    pGetDpiForSystem
        = reinterpret_cast<GetDpiForSystemFunc>(GetProcAddress(hUser32, "GetDpiForSystem"));

    return 0;
}

void SupportDPI()
{
    if (Snapshot::Platform::IsWin10_1607OrGreater())
    {
        if (pSetThreadDpiAwarenessContext)
        {
            pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }
    else if (Snapshot::Platform::IsWindows8Point1OrGreater())
    {
        PROCESS_DPI_AWARENESS ctx{ PROCESS_PER_MONITOR_DPI_AWARE };
        SetProcessDpiAwareness(ctx);
    }
    else
    {
        SetProcessDPIAware();
    }
}

int GetDpiForWindow(HWND hWnd)
{
    int dpi = USER_DEFAULT_SCREEN_DPI;
    if (pGetDpiForWindow)
        dpi =  pGetDpiForWindow(hWnd);
    return dpi;
}

int GetDpiForSystem()
{
    int dpi = USER_DEFAULT_SCREEN_DPI;
    if (pGetDpiForSystem)
        dpi = pGetDpiForSystem();
    return dpi;
}

int GuessSystemDpi(HWND hWnd)
{
    auto hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFOEX mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    CRect rm{ mi.rcMonitor };
    int cxLogical = rm.Width();
    int cyLogical = rm.Height();

    DEVMODE dm;
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    EnumDisplaySettings(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    int phyWidth = (int)dm.dmPelsWidth;
    int phyHeight = (int)dm.dmPelsHeight;
    POINTL pt = dm.dmPosition;

    int v = ((double)phyWidth / (double)cxLogical) * USER_DEFAULT_SCREEN_DPI;

    return v;
}

UINT GetMonitorDpiByWindow(HWND hWnd)
{
    auto hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
    UINT dpiX, dpiY;
    if (FAILED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
    {
        dpiX = USER_DEFAULT_SCREEN_DPI;
    }

    return dpiX;
}

#pragma comment(lib, "dxgi.lib")

UINT GetMonitorDXGIWidth(HMONITOR hm)
{
    using namespace Microsoft::WRL;
    ComPtr<IDXGIFactory> factory;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory);
    ComPtr<IDXGIAdapter> hDxgiAdapter = NULL;
    UINT n = 0;
    bool bFound = false;
    ComPtr<IDXGIOutput> hDxgiOutput = NULL;
    CRect dxgiRect{};

    while (factory->EnumAdapters(n, &hDxgiAdapter) == S_OK)
    {
        UINT nOutput = 0;
        do
        {
            hr = hDxgiAdapter->EnumOutputs(nOutput, &hDxgiOutput);
            if (hr == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }

            DXGI_OUTPUT_DESC eDesc = {};
            hDxgiOutput->GetDesc(&eDesc);
            dxgiRect = eDesc.DesktopCoordinates;
            if (eDesc.Monitor == hm)
            {
                bFound = true;
                break;
            }
            ++nOutput;
        } while (SUCCEEDED(hr));

        if (bFound)
        {
            break;
        }
        n++;
    }

    return dxgiRect.Width();
}

} // namespace DPI

} // namespace Snapshot

