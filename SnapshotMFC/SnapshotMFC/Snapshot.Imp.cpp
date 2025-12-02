#include "pch.h"
#include "Snapshot.Imp.h"

#include "Snapshot.Utils.h"
#include "Snapshot.DPI.Utils.h"
#include "WinVersionHelper.h"

#include "logger.h"
#include <sstream>

namespace Snapshot
{
namespace ImpGDI
{
static HBITMAP _create32bitRGBDIB(int width, int height, void *&pbmp)
{
    BITMAPINFO bmi{ 0 };
    BITMAPINFOHEADER &bmh = bmi.bmiHeader;
    memset(&bmi, 0, sizeof(BITMAPINFOHEADER));
    bmh.biSize = sizeof(BITMAPINFOHEADER);
    bmh.biWidth = width;
    bmh.biHeight = height;
    bmh.biPlanes = 1;
    bmh.biBitCount = 32;
    bmh.biCompression = BI_RGB;
    auto hb = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pbmp, NULL, 0);

    return hb;
}

int CaptureAnImageGDI(HWND hWnd, int reqWidth, int reqHeight, snap_img_desc &dest)
{
    int ret = -1;
    HDC hdc = ::GetDC(hWnd);
    HDC cdc = CreateCompatibleDC(hdc);

    CSize exportSz{ reqWidth, reqHeight };
    CRect rectdwm;
    auto dwmret = DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rectdwm, sizeof(RECT)) == S_OK;
    WINDOWINFO info{};
    info.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(hWnd, &info);
    UINT dpi = USER_DEFAULT_SCREEN_DPI;
    UINT dpin = Snapshot::DPI::GuessSystemDpi(hWnd);

    auto nrec = rectdwm.MulDiv(dpi, dpin);

    //logger::logInfoW(L"hwnd/monitor/system: %d/%d/%d/%d", dpi, dpim, dpis, dpin);
    logger::logInfoW(L"f/s/r: %dx%d / %dx%d / %dx%d", rectdwm.Width(), rectdwm.Height(),
                     nrec.Width(), nrec.Height(), reqWidth, reqHeight);

    auto dr = Snapshot::Utils::RectPadding(CSize{ nrec.Width(), nrec.Height() }, exportSz);

    void *pbmpl1{ nullptr };
    HGDIOBJ hb{ nullptr }, hOldObj{ nullptr };

    UINT pwflag = 0;

    void *pbmpl2{ nullptr };
    HDC cdc2{ nullptr };
    HGDIOBJ hb2{ nullptr }, hOldObj2{ nullptr };

    do
    {
        if (!(cdc && hdc))
        {
            logger::logInfoW(L" GDI can not get dc");
            break;
        }

        pwflag = Platform::IsWindows8Point1OrGreater() ? PW_RENDERFULLCONTENT : 0;
        hb = _create32bitRGBDIB(nrec.Width(), -nrec.Height(), pbmpl1);
        if (!hb)
        {
            logger::logInfoW(L" GDI  No free memory for dib");
            break;
        }

        auto hOldObj = SelectObject(cdc, hb);
        if (::PrintWindow(hWnd, cdc, pwflag) != TRUE)
        {
            logger::logInfoW(L" GDI can not get content, try ");

            SetStretchBltMode(cdc, HALFTONE);
            auto bRet = StretchBlt(cdc, dr.left, dr.top, dr.Width(), dr.Height(), hdc, 0, 0,
                                   rectdwm.Width(), rectdwm.Height(), SRCCOPY)
                != 0;
        }

        cdc2 = CreateCompatibleDC(cdc);
        hb2 = _create32bitRGBDIB(exportSz.cx, -exportSz.cy, pbmpl2);
        if (!(cdc2 && hb2))
        {
            logger::logInfoW(L" GDI No free memory for dib2");
            break;
        }

        hOldObj2 = SelectObject(cdc2, hb2);
        CRect fr{ 0, 0, exportSz.cx, exportSz.cy };
        FillRect(cdc2, fr, (HBRUSH)GetStockObject(WHITE_BRUSH));
        SetStretchBltMode(cdc2, HALFTONE);
        auto loffset = info.cxWindowBorders;
        auto bRet = StretchBlt(cdc2, dr.left, dr.top, dr.Width(), dr.Height(), cdc, loffset, 0,
                               nrec.Width() - loffset, nrec.Height(), SRCCOPY)
            != 0;

        /*Snapshot::Utils::save_bmp((uint8_t *)pbmpl2, exportSz.cx, exportSz.cy, 4,
                                    exportSz.cx, exportSz.cy, path);*/
        Snapshot::Utils::save_png_mem((uint8_t *)pbmpl2, exportSz.cx, exportSz.cy, 4, exportSz.cx,
                                      exportSz.cy, dest.data, dest.data_len);
        ret = 0;
    } while (0);

    if (cdc2)
    {
        if (hOldObj2)
            SelectObject(cdc2, hOldObj2);
        DeleteDC(cdc2);
    }

    if (hb2)
        DeleteObject(hb2);

    if (hb)
        DeleteObject(hb);

    if (cdc)
    {
        if (hOldObj)
            SelectObject(cdc, hOldObj);
        DeleteDC(cdc);
    }

    if (hdc)
        ReleaseDC(hWnd, hdc);
   
    return ret;
}

int CaptureAnImageGDIFromDesktop(HWND hWnd, int reqWidth, int reqHeight, snap_img_desc &dest)
{
    HDC hdc;
    HDC cdc = NULL;
    void *lpbitmap = nullptr;

    CSize exportSz{ reqWidth, reqHeight };

    auto previousHwnd = ::GetForegroundWindow();
    ::SetForegroundWindow(hWnd);
    ::BringWindowToTop(hWnd);

    HMONITOR hm = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX mInfo;
    ZeroMemory(&mInfo, sizeof(MONITORINFOEX));
    mInfo.cbSize = sizeof(MONITORINFOEX);

    GetMonitorInfoW(hm, &mInfo);

    hdc = CreateDCW(L"myDisplay", mInfo.szDevice, NULL, NULL);
    cdc = CreateCompatibleDC(hdc);
    if (cdc)
    {
        CRect rect;
        DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(RECT));
        auto hbmScreen = _create32bitRGBDIB(exportSz.cx, -exportSz.cy, lpbitmap);
        if (hbmScreen)
        {
            auto hOldBitmap = SelectObject(cdc, hbmScreen);
            auto dr = Snapshot::Utils::RectPadding(CSize{ rect.Width(), rect.Height() }, exportSz);
            CRect fr{ 0, 0, exportSz.cx, exportSz.cy };
            FillRect(cdc, fr, (HBRUSH)GetStockObject(WHITE_BRUSH));
            SetStretchBltMode(cdc, HALFTONE);
            auto bRet = StretchBlt(cdc, dr.left, dr.top, dr.Width(), dr.Height(), hdc, rect.left,
                                   rect.top, rect.Width(), rect.Height(), SRCCOPY)
                == TRUE;

            //Snapshot::Utils::save_bmp((uint8_t *)lpbitmap, exportSz.cx, exportSz.cy, 4, exportSz.cx,
            //                          exportSz.cy, dest);

            Snapshot::Utils::save_png_mem((uint8_t *)lpbitmap, exportSz.cx, exportSz.cy, 4,
                                          exportSz.cx, exportSz.cy, dest.data, dest.data_len);
            SelectObject(cdc, hOldBitmap);
            DeleteObject(hbmScreen);
        }
    }

    SetForegroundWindow(previousHwnd);

    if (hdc)
        DeleteObject(hdc);
    if (cdc)
        DeleteDC(cdc);

    return 0;
}

/* 被打印窗体 可以最小化
 * 必须是顶层窗体（不能是子控件）
 */

static HWND thumbWin = nullptr;

int CaptureAnImageDWM(HWND hWnd, int reqWidth, int reqHeight, snap_img_desc &dest)
{
    int ret = -1;
    if (!IsWindow(hWnd))
        return ret;

    BOOL enabled = FALSE;
    if (FAILED(DwmIsCompositionEnabled(&enabled)) || !enabled)
    {
        return ret;
    }

    UINT dpi = USER_DEFAULT_SCREEN_DPI;
    UINT dpin = Snapshot::DPI::GuessSystemDpi(hWnd);
    logger::logInfoW(L"radio %d/%d", dpi, dpin);

    const int x
        = (GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN)) * dpi / dpin;
    const int y
        = (GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN)) * dpi / dpin;

    if (thumbWin == nullptr)
        thumbWin = CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"", WS_POPUP | CS_DROPSHADOW, x, y,
                                   10, 10, nullptr, nullptr, nullptr, nullptr);
    HTHUMBNAIL thumbnail{ nullptr };

    HDC hdc{ nullptr };
    HDC cdc{ nullptr };
    HBITMAP hb{ nullptr };
    void *pbmpl1{ nullptr };
    HGDIOBJ hobj{ nullptr };

    HDC cdc2{ nullptr };
    HBITMAP hb2{ nullptr };
    void *pbmpl2{ nullptr };
    HGDIOBJ hobj2{ nullptr };

    do
    {
        if (!thumbWin)
        {
            logger::logInfoW(L"no thumbnail wnd impl", hWnd);
            break;
        }

        HRESULT hr = DwmRegisterThumbnail(thumbWin, hWnd, &thumbnail);
        if (FAILED(hr))
        {
            logger::logInfoW(L"can not register thumbnail for %x", hWnd);
            break;
        }

        CSize viewportSz{ reqWidth, reqHeight };
        SIZE size{};
        hr = DwmQueryThumbnailSourceSize(thumbnail, &size);
        auto dr = Snapshot::Utils::RectPadding(CSize{ size.cx, size.cy }, viewportSz);
        logger::logInfoW(L"Thumbnail %x: f/s: %dx%d / %dx%d", hWnd, size.cx, size.cy, dr.Width(),
                 dr.Height());
        if (FAILED(hr))
        {
            logger::logInfoW(L"can not Query thumbnail %x", hWnd);
            break;
        }

        logger::logInfoW(L"move window to %dx%d, %dx%d", x, y, reqWidth, reqHeight);
        SetWindowPos(thumbWin, nullptr, x, y, reqWidth, reqHeight,
                     SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        ShowWindow(thumbWin, SW_SHOW);

        DWM_THUMBNAIL_PROPERTIES dskThumbProps;
        dskThumbProps.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE
            | DWM_TNP_SOURCECLIENTAREAONLY | DWM_TNP_OPACITY;
        dskThumbProps.fSourceClientAreaOnly = FALSE;
        dskThumbProps.fVisible = TRUE;
        dskThumbProps.opacity = 255;
        dskThumbProps.rcDestination = dr;
        hr = DwmUpdateThumbnailProperties(thumbnail, &dskThumbProps);
        if (FAILED(hr))
        {
            logger::logInfoW(L"can not update properties");
            break;
        }

        hdc = ::GetDC(thumbWin);
        cdc = CreateCompatibleDC(hdc);
        hb = _create32bitRGBDIB(viewportSz.cx, -viewportSz.cy, pbmpl1);
        if (!(cdc && hdc && hb))
        {
            logger::logInfoW(L"can not alloc memory for dc");
            break;
        }

        UINT flag = Snapshot::Platform::IsWindows8Point1OrGreater() ? PW_RENDERFULLCONTENT : 0;
        hobj = SelectObject(cdc, hb);
        if (::PrintWindow(thumbWin, cdc, flag) != TRUE)
        {
            logger::logInfoW(L"print window error");
            break;
        }

        cdc2 = CreateCompatibleDC(cdc);
        hb2 = _create32bitRGBDIB(viewportSz.cx, -viewportSz.cy, pbmpl2);
        if (!(cdc2 && hb2))
        {
            logger::logInfoW(L"can not alloc memory for dc2");
            break;
        }

        hobj2 = SelectObject(cdc2, hb2);
        CRect fr{ 0, 0, viewportSz.cx, viewportSz.cy };
        FillRect(cdc2, fr, (HBRUSH)GetStockObject(WHITE_BRUSH));
        SetStretchBltMode(cdc2, HALFTONE);
        auto sbret = StretchBlt(cdc2, dr.left, dr.top, dr.Width(), dr.Height(), cdc, dr.left,
                                dr.top, dr.Width(), dr.Height(), SRCCOPY) != 0;
        if (!sbret)
        {
            logger::logInfoW(L"can not StretchBlt");
            break;
        }

        /*
        std::wostringstream wss;
        static int i = 0;
        wss << LR"(D:/temp/)" << (uint32_t)thumbWin << "." << i++ <<
        L"("
            << reqWidth << "x" << reqHeight << L")" << L".png";
        Snapshot::Utils::save_png((uint8_t *)pbmpl1, reqWidth,
        reqHeight, 4, reqWidth, reqHeight, wss.str());

        */
        Snapshot::Utils::save_png_mem((uint8_t *)pbmpl2, viewportSz.cx, viewportSz.cy, 4,
                                      viewportSz.cx, viewportSz.cy, dest.data, dest.data_len);
    } while (0);

    if (thumbnail)
        DwmUnregisterThumbnail(thumbnail);

    if (cdc2)
    {
        if (hobj2)
            SelectObject(cdc2, hobj2);
        DeleteDC(cdc2);
    }
    if (hb2)
        DeleteObject(hb2);

    if (cdc)
    {
        if (hobj)
            SelectObject(cdc, hobj);
        DeleteDC(cdc);
    }

    if (hb)
        DeleteObject(hb);

    if (hdc)
        DeleteDC(hdc);

    return ret;
}
#include <shellscalingapi.h>

int CaptureMonitorByGDI(HMONITOR hMonitor, int reqWidth, int reqHeight, snap_img_desc &dest)
{
    Snapshot::DPI::SupportDPI();
    CSize exportSz{ reqWidth, reqHeight };

    MONITORINFOEX mInfo{};
    mInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfoW(hMonitor, &mInfo);
    HDC hdc{ nullptr };
    HDC cdc{ nullptr };
    HBITMAP hbm{ nullptr };
    void *lpbitmap{ nullptr };
    HGDIOBJ hOldObj{ nullptr };

    do
    {
        hdc = CreateDCW(L"myDisplay", mInfo.szDevice, NULL, NULL);
        cdc = CreateCompatibleDC(hdc);
        if (hdc && cdc)
        {
            CRect rect{ mInfo.rcMonitor };
            hbm = _create32bitRGBDIB(exportSz.cx, -exportSz.cy, lpbitmap);
            if (!hbm)
            {
                logger::logInfoW(L"can not alloc memory for hdib");
                break;
            }
            hOldObj = SelectObject(cdc, hbm);
            auto dr = Snapshot::Utils::RectPadding(CSize{ rect.Width(), rect.Height() }, exportSz);
            CRect fr{ 0, 0, exportSz.cx, exportSz.cy };
            FillRect(cdc, fr, (HBRUSH)GetStockObject(WHITE_BRUSH));
            SetStretchBltMode(cdc, HALFTONE);
            auto bRet = StretchBlt(cdc, dr.left, dr.top, dr.Width(), dr.Height(), hdc, 0,
                                   0, rect.Width(), rect.Height(), SRCCOPY)
                != 0;
            if (!bRet)
            {
                logger::logInfoW(L"can not alloc memory for dc2");
                break;
            }

            // Snapshot::Utils::save_bmp((uint8_t *)lpbitmap, exportSz.cx, exportSz.cy, 4,
            // exportSz.cx,
            //                           exportSz.cy, dest);

            Snapshot::Utils::save_png_mem((uint8_t *)lpbitmap, exportSz.cx, exportSz.cy, 4,
                                          exportSz.cx, exportSz.cy, dest.data, dest.data_len);
        }
    } while (0);

    if (hbm)
        DeleteObject(hbm);

    if (hdc)
    {
        if (hOldObj)
        {
            SelectObject(hdc, hOldObj);
        }
            
        DeleteObject(hdc);
    }

    if (cdc)
        DeleteDC(cdc);
    return 0;
}

int FreeImage(snap_img_desc &desc)
{
    Snapshot::Utils::free_mem(desc.data, desc.data_len);
    return 0;
}

} // namespace ImpGDI
} // namespace Snapshot
