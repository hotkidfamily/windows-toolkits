#pragma once

typedef struct tagHookContext
{
    HWND hWnd{ nullptr };
    HHOOK hHook{ nullptr };
}HookContext;

extern HookContext g_hookCtx;
LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);

class WndKeeper {
  public:
    WndKeeper(HWND hWnd)
    {
        auto hHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, NULL, GetCurrentThreadId());
        if (hHook)
        {
            g_hookCtx.hHook = hHook;
            g_hookCtx.hWnd = hWnd;
        }
            
    }

    ~WndKeeper()
    {
        if (g_hookCtx.hHook)
        {
            UnhookWindowsHookEx(g_hookCtx.hHook);
            g_hookCtx.hHook = nullptr;
        }
    }
};
