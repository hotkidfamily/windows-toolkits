#include "pch.h"
#include "WndKeeper.h"

HookContext g_hookCtx;

LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        CWPSTRUCT *cwp = (CWPSTRUCT *)lParam;

        if (cwp->message == WM_MOVING)
        {
            if (cwp->hwnd == g_hookCtx.hWnd)
            {
                RECT *pRect = (RECT *)cwp->lParam;
                RECT screenRect;
                SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);

                int screenWidth = screenRect.right - screenRect.left;
                int allowedLeft = screenRect.left + screenWidth / 2;

                if (pRect->left < allowedLeft)
                {
                    int width = pRect->right - pRect->left;
                    pRect->left = allowedLeft;
                    pRect->right = allowedLeft + width;
                }
            }
        }
    }

    return CallNextHookEx(g_hookCtx.hHook, nCode, wParam, lParam);
}
