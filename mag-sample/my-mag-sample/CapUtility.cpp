#include "stdafx.h"
#include "CapUtility.h"

#include <shellscalingapi.h>
#include <set>
#include "WinVersionHelper.h"

namespace CapUtility
{
static HMODULE hShcoreModule = LoadLibraryW(L"Shcore.dll");
typedef HRESULT(__stdcall *funcGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);

BOOL _getDpiForMonitor(HMONITOR hm, UINT *dpix, UINT *dpiy)
{
    HRESULT hRet = E_FAIL;
    static funcGetDpiForMonitor _ptrGetDpiForMonitor = nullptr;
    if (!_ptrGetDpiForMonitor) {
        if (hShcoreModule) {
            _ptrGetDpiForMonitor
                = reinterpret_cast<funcGetDpiForMonitor>(GetProcAddress(hShcoreModule, "GetDpiForMonitor"));
        }
    }
    else {
        hRet = _ptrGetDpiForMonitor(hm, MDT_RAW_DPI, dpix, dpiy);
    }

    return SUCCEEDED(hRet);
}


typedef HRESULT(__stdcall *funcGetScaleFactorForMonitor)(HMONITOR hmonitor, DEVICE_SCALE_FACTOR *scale);

BOOL _getScaleForMonitor(HMONITOR hm, DEVICE_SCALE_FACTOR *scale)
{
    HRESULT hRet = E_FAIL;
    static funcGetScaleFactorForMonitor _ptrGetScaleFactorForMonitor = nullptr;
    if (!_ptrGetScaleFactorForMonitor) {
        if (hShcoreModule) {
            _ptrGetScaleFactorForMonitor = reinterpret_cast<funcGetScaleFactorForMonitor>(
                GetProcAddress(hShcoreModule, "GetScaleFactorForMonitor"));
        }
    }
    else {
        hRet = _ptrGetScaleFactorForMonitor(hm, scale);
    }

    return SUCCEEDED(hRet);
}

BOOL isWndCanCap(HWND hWnd)
{
    return IsWindow(hWnd)        // 是窗口
        && IsWindowVisible(hWnd) // 可见的
        && !IsIconic(hWnd);      // 非最小化的
}

std::vector<HWND> getWindowsCovered(HWND hTartgetWnd)
{
    std::set<HWND> wndlist;
    RECT rcTarget;
    GetWindowRect(hTartgetWnd, &rcTarget);
    HWND enumWnd = hTartgetWnd;

    while (NULL != (enumWnd = GetNextWindow(enumWnd, GW_HWNDPREV))) {
        int style = GetWindowLong(enumWnd, GWL_STYLE);
        int exStyle = GetWindowLong(enumWnd, GWL_EXSTYLE);

        if (isWndCanCap(enumWnd))
        {
            HWND rootWnd = enumWnd;

            if (style & WS_POPUP) {
                if (hTartgetWnd == (rootWnd = GetAncestor(enumWnd, GA_ROOTOWNER))) { // 如果 owner 和 目标是一样的，不过滤
                    continue;
                }
                else {
                    rootWnd = enumWnd;
                }
            }

            RECT rcWnd;
            GetWindowRect(rootWnd, &rcWnd);
            if (!IsRectEmpty(&rcWnd)) {
                if (!((rcWnd.right < rcTarget.left) || (rcWnd.left > rcTarget.right) || (rcWnd.bottom < rcTarget.top)
                      || (rcWnd.top > rcTarget.bottom))) {
                    wndlist.insert(rootWnd);
                }
            }
        }
    }

    std::vector<HWND> wndList2;
    if (wndlist.size()){
        for (auto &ele : wndlist) {
            wndList2.push_back(ele);
        }
    }

    return wndList2;
}


std::vector<DisplaySetting> _enumDisplaySetting()
{
    DISPLAY_DEVICEW device = { 0 };
    device.cb = sizeof(DISPLAY_DEVICEW);

    std::vector<DisplaySetting> settings;

    for (int index = 0;; index++) {
        if (!EnumDisplayDevicesW(NULL, index, &device, EDD_GET_DEVICE_INTERFACE_NAME))
            break;

        DEVMODEW devmode = { 0 };
        devmode.dmSize = sizeof(DEVMODEW);
        // for (int modes = 0;; modes++) {
        if (!EnumDisplaySettingsW(device.DeviceName, ENUM_CURRENT_SETTINGS, &devmode))
            continue;
        //}
        devmode = devmode;
        DisplaySetting m(device, devmode);
        settings.push_back(m);
    }

    return settings;
}


bool getMaxResolutionInSystem(int32_t *cx, int32_t *cy)
{
    std::vector<DisplaySetting> settings = _enumDisplaySetting();
    int32_t maxWidth = 0;
    int32_t maxHeight = 0;

    if (settings.size()) {
        for (auto &s : settings) {
            CRect rect(s.rect());
            if (maxWidth < rect.Width()) {
                maxWidth = rect.Width();
            }
            if (maxHeight < rect.Height()) {
                maxHeight = rect.Height();
            }
        }
    }
    else {
        maxWidth = 1920;
        maxHeight = 1080;
    }

    *cx = maxWidth;
    *cy = maxHeight;

    return true;
}

DisplaySetting enumDisplaySettingByName(std::wstring &name)
{
    std::vector<DisplaySetting> settings = _enumDisplaySetting();

    for (auto &set : settings) 
    {
        if (set.name() == name) {
            DisplaySetting m = set;
            return m;
        }
    }

    return nullptr;
}

DisplaySetting enumDisplaySettingByMonitor(HMONITOR hMonitor)
{
    MONITORINFOEXW minfo;
    minfo.cbSize = sizeof(MONITORINFOEXW);
    GetMonitorInfo(hMonitor, &minfo);
    std::wstring str(minfo.szDevice);
    CapUtility::DisplaySetting settings = CapUtility::enumDisplaySettingByName(str);
    return settings;
}

static int(WINAPI *ptrGetSystemMetricsForDpi)(int, UINT) = NULL;
static UINT(WINAPI *pfnGetDpiForSystem)() = NULL;
static UINT(WINAPI *pfnGetDpiForWindow)(HWND) = NULL;

UINT GetDPI(HWND hWnd)
{
    if (hWnd != NULL) {
        if (pfnGetDpiForWindow)
            return pfnGetDpiForWindow(hWnd);
    }
    else {
        if (pfnGetDpiForSystem)
            return pfnGetDpiForSystem();
    }
    if (HDC hDC = GetDC(hWnd)) {
        auto dpi = GetDeviceCaps(hDC, LOGPIXELSX);
        ReleaseDC(hWnd, hDC);
        return dpi;
    }
    else
        return USER_DEFAULT_SCREEN_DPI;
}

static int metrics[SM_CMETRICS] = { 0 };

template <typename P>
bool Symbol(HMODULE h, P &pointer, const char *name)
{
    if (P p = reinterpret_cast<P>(GetProcAddress(h, name))) {
        pointer = p;
        return true;
    }
    else
        return false;
}

LRESULT UpdateSysmteMetrics(HWND hWnd)
{
    HMODULE hUser32 = GetModuleHandle(L"USER32");
    Symbol(hUser32, pfnGetDpiForSystem, "GetDpiForSystem");
    Symbol(hUser32, pfnGetDpiForWindow, "GetDpiForWindow");
    Symbol(hUser32, ptrGetSystemMetricsForDpi, "GetSystemMetricsForDpi");

    long dpi = GetDPI(hWnd);
    UINT dpiSystem = GetDPI(NULL);

    if (ptrGetSystemMetricsForDpi) {
        for (auto i = 0; i != sizeof metrics / sizeof metrics[0]; ++i) {
            metrics[i] = ptrGetSystemMetricsForDpi(i, dpi);
        }
    }
    else {
        for (auto i = 0; i != sizeof metrics / sizeof metrics[0]; ++i) {
            metrics[i] = dpi * GetSystemMetrics(i) / dpiSystem;
        }
    }
    return 0;
}

LRESULT GetPrimeryWindowsRect(RECT &rect)
{
    UpdateSysmteMetrics(NULL);

    rect.left = 0;
    rect.top = 0;

    rect.right = metrics[SM_CXSCREEN];
    rect.bottom = metrics[SM_CYSCREEN];

    return 0;
}


BOOL GetWindowRectAccuracy(HWND hWnd, RECT &iRect)
{
    BOOL bRet = FALSE;
    if (Platform::IsWindows8OrGreater()) {
        bRet = S_OK == DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &iRect, sizeof(RECT));
    }
    else if (Platform::IsWindows7OrGreater()){
        BOOL dwmEnabled = FALSE;
        if (SUCCEEDED(DwmIsCompositionEnabled(&dwmEnabled))) {
            bRet = S_OK == DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &iRect, sizeof(RECT));
        }
        else {
            WINDOWINFO info;
            info.cbSize = sizeof(WINDOWINFO);
            CRect rect;
            ::GetWindowRect(hWnd, rect);
            ::GetWindowInfo(hWnd, &info);

            if (IsZoomed(hWnd)) {
                rect.top += info.cyWindowBorders;
                rect.left += info.cxWindowBorders;
                rect.bottom -= info.cyWindowBorders;
                rect.right -= info.cxWindowBorders;
            }
            else if (!(info.dwStyle & WS_THICKFRAME)) {
                rect.top += info.cyWindowBorders / 4;
                rect.left += info.cxWindowBorders / 4;
                rect.bottom -= info.cyWindowBorders / 4;
                rect.right -= info.cxWindowBorders / 4;
            }
            else if (info.dwStyle & WS_OVERLAPPEDWINDOW) {
                rect.left += info.cxWindowBorders;
                rect.bottom -= info.cyWindowBorders;
                rect.right -= info.cxWindowBorders;
            }
            iRect = rect;
            bRet = TRUE;
        }
    }

    if( !bRet ){
        GetWindowRect(hWnd, &iRect);
        bRet = TRUE;
    }

    return bRet;
}

};
