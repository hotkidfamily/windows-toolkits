#pragma once

#include <dwmapi.h>
#include <string>
#include <array>
#include <vector>

struct Window
{
  public:
    Window(nullptr_t)
    {
    }
    Window(HWND hwnd, std::wstring const &title, std::wstring &className, DWORD pid, bool transparent = false)
    {
        _hwnd = hwnd;
        _title = title;
        _className = className;
        _transparent = transparent;
        _pid = pid;
    }

    HWND Hwnd() const noexcept
    {
        return _hwnd;
    }
    std::wstring Title() const noexcept
    {
        return _title;
    }
    std::wstring ClassName() const noexcept
    {
        return _className;
    }
    auto Transparent() const noexcept
    {
        return _transparent;
    }
    DWORD PID() const noexcept
    {
        return _pid;
    }

  private:
    HWND _hwnd;
    std::wstring _title;
    std::wstring _className;
    bool _transparent;
    DWORD _pid = -1;
};

inline std::wstring GetClassName(HWND hwnd)
{
    std::array<WCHAR, 1024> className;

    ::GetClassName(hwnd, className.data(), (int)className.size());

    std::wstring title(className.data());
    return title;
}

inline std::wstring GetWindowText(HWND hwnd)
{
    std::array<WCHAR, 1024> windowText;

    ::GetWindowText(hwnd, windowText.data(), (int)windowText.size());

    std::wstring title(windowText.data());
    return title;
}

inline bool IsWindowExcludeFromCapture(HWND hwnd)
{
    DWORD affinity = 0;
    auto ret = GetWindowDisplayAffinity(hwnd, &affinity);
    return (ret == TRUE) && (affinity == WDA_EXCLUDEFROMCAPTURE);
}

inline bool IsWindowDisabled(HWND hwnd)
{
    auto style = GetWindowLong(hwnd, GWL_STYLE);
    return (style & WS_DISABLED) == WS_DISABLED;
}

inline bool IsTransparentWindow(HWND hwnd)
{
    auto style = GetWindowLong(hwnd, GWL_EXSTYLE);
    return (style & WS_EX_TRANSPARENT) == WS_EX_TRANSPARENT;
}

inline bool IsExLayderedWindow(HWND hwnd)
{
    auto style = GetWindowLong(hwnd, GWL_EXSTYLE);
    return (style & WS_EX_LAYERED) == WS_EX_LAYERED;
}

inline bool IsToolWindow(HWND hwnd)
{
    auto style = GetWindowLong(hwnd, GWL_EXSTYLE);
    return (style & WS_EX_TOOLWINDOW) == WS_EX_TOOLWINDOW;
}

inline bool IsNoActivateWindow(HWND hwnd)
{
    auto style = GetWindowLong(hwnd, GWL_EXSTYLE);
    return (style & WS_EX_NOACTIVATE) == WS_EX_NOACTIVATE;
}

inline bool IsWindowCloaked(Window wnd)
{
    bool ret = false;

    DWORD cloaked = FALSE;
    HRESULT hrTemp = DwmGetWindowAttribute(wnd.Hwnd(), DWMWA_CLOAKED, &cloaked, sizeof(cloaked));

    ret = SUCCEEDED(hrTemp) && cloaked;

    return ret;
}

inline bool IsInvalidWindowSize(HWND hwnd)
{
    CRect rect;
    GetWindowRect(hwnd, &rect);
    return !!IsRectEmpty(&rect);
}

static bool IsWindowCapable(Window wnd)
{
    HWND hwnd = wnd.Hwnd();
    HWND shell = GetShellWindow();
    HWND desktop = GetDesktopWindow();

    if (hwnd == shell) {
        return false;
    }

    if (wnd.Title().length() == 0) {
        return false;
    }

    if (!IsWindow(hwnd)) {
        return false;
    }

    if (!IsWindowVisible(hwnd)) {
        return false;
    }

    if (IsWindowCloaked(wnd)) {
        return false;
    }

    if (IsWindowExcludeFromCapture(hwnd)) {
        return false;
    }

    if (IsIconic(hwnd)) {
        return false;
    }

    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD) {
        HWND hParent = GetParent(hwnd);
        if (hParent != desktop) {
            return false;
        }
    }
    else {
        HWND hOwner = GetWindow(hwnd, GW_OWNER);
        if (hOwner != nullptr) {
            return false;
        }
    }

    if (GetAncestor(hwnd, GA_ROOT) != hwnd) {
        return false;
    }

    if (GetAncestor(hwnd, GA_ROOTOWNER) != hwnd) {
        return false;
    }

    if (IsInvalidWindowSize(hwnd)) {
        return false;
    }

    //if (IsWindowDisabled(hwnd)) {
    //    return false;
    //}

    // if (IsToolWindow(hwnd)) {
    //     return false;
    // }

    return true;
}

static bool IsInBlackList(Window &wnd)
{
    auto cn = wnd.ClassName();
    if (cn.find(L"Huya") == std::wstring::npos) {
        return false;
    }
    else if (cn.find(L"HwndWrapper[HuyaClient;;484d2019-0f6a-42b5-88d7-feb70478a643]") != std::wstring::npos) {
        return false;
    }

    return true;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto class_name = GetClassName(hwnd);
    auto title = GetWindowText(hwnd);
    auto transparent = IsTransparentWindow(hwnd);
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    auto window = Window(hwnd, title, class_name, pid, transparent);

    if (!IsWindowCapable(window)) {
        return TRUE;
    }

    //if (IsInBlackList(window)) {
    //    return TRUE;
    //}

    std::vector<Window> &windows = *reinterpret_cast<std::vector<Window> *>(lParam);
    windows.push_back(window);

    return TRUE;
}

const std::vector<Window> EnumerateWindows()
{
    std::vector<Window> windows;

    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));

    return windows;
}