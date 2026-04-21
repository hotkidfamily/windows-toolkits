#include "stdafx.h"
#include "DWMCapture.h"

#include <functional>
#include <chrono>

#include "CapUtility.h"
#include "CPGPU.h"

#include "logger.h"
#include "WinVersionHelper.h"


DWMCapture::DWMCapture()
{
    _exit = false;
    _thread = std::thread(std::bind(&DWMCapture::_run, this));
}

DWMCapture::~DWMCapture()
{
    stop();

    PostMessage(_hostWnd, WM_QUIT, NULL, NULL);
    DestroyWindow(_hostWnd);
    _hostWnd = nullptr;

    _exit = true;
    if (_thread.joinable()) {
        _thread.join();
    }
}

bool DWMCapture::startCaptureWindow(HWND hWnd)
{
    _hwnd = hWnd;
    _hmonitor = nullptr;

    _action = ACTION::ACTION_Start;

    return true;
}

bool DWMCapture::startCaptureScreen(HMONITOR hMonitor)
{
    _hwnd = nullptr;
    _hmonitor = hMonitor;

    _action = ACTION::ACTION_Start;
    return true;
}

bool DWMCapture::stop()
{
    _action = ACTION::ACTION_Stop;
    return true;
}

bool DWMCapture::_onCaptured(uint8_t* data, CRect &vp)
{
    bool bRet = false;
    int x = abs(_lastRect.left());
    int y = abs(_lastRect.top());
    int width = _lastRect.width();
    int height = _lastRect.height();
    int stride = width * 4;
    auto inStride = vp.Width()*4;

    if (_lastRect.is_empty()) {
        width = vp.Width();
        height = vp.Height();
        stride = width * 4;
    }

    uint8_t *pBits = (uint8_t *)data;
    DesktopRect capRect = DesktopRect::MakeXYWH(0, 0, vp.Width(), vp.Height());

    int bpp = CapUtility::kDesktopCaptureBPP;
    if (!_frames.get() || width != _frames->width() || height != _frames->height() || stride != _frames->stride()
        || bpp != CapUtility::kDesktopCaptureBPP) {
        _frames.reset(VideoFrame::MakeFrame(width, height, stride, VideoFrameType::kVideoFrameTypeRGBA));
    }

    {
        uint8_t *pDst = reinterpret_cast<uint8_t *>(_frames->data());
        uint8_t *pSrc = reinterpret_cast<uint8_t *>(pBits);

        for (int i = 0; i < height; i++) {
            memcpy(pDst, pSrc, stride);
            pDst += stride;
            pSrc += inStride;
        }
    }

    invokeCallback(_frames.get());

    return bRet;
}



void DWMCapture::_run()
{
    bool success = false;
    auto frame_interval = std::chrono::milliseconds(1000 / _fps);

    while (!_exit) {
        switch (_action) {
        case ACTION::ACTION_Start:
            _stopSession();
            _action = ACTION::ACTION_Busy;
            _createSession();
            break;
        case ACTION::ACTION_Stop:
            _action = ACTION::ACTION_Idle;
            _stopSession();
            break;
        case ACTION::ACTION_Busy:
        {
            auto start = std::chrono::high_resolution_clock::now();

            _updateThumbnailSize();

            DesktopRect rect = getCaptureRect();
            _lastRect = rect;
            CaptureAnImageGDI();

            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            auto remaining = frame_interval - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            if (remaining.count() > 0) {
                std::this_thread::sleep_for(remaining);
            }
            break;
        }
        default:
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            break;
        }
    }
}

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

int DWMCapture::CaptureAnImageGDI()
{
    HWND hWnd = _hostWnd;
    HDC hdc = ::GetDC(hWnd);
    HDC cdc = CreateCompatibleDC(hdc);

    CRect rectdwm;
    auto dwmret = DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rectdwm, sizeof(RECT)) == S_OK;
    WINDOWINFO info{};
    info.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(hWnd, &info);
    UINT dpi = GetDpiForWindow(hWnd);
    UINT dpis = GetDpiForSystem();

    auto nrec = rectdwm.MulDiv(dpi, dpis);

    CSize exportSz{ _lastRect.width(), _lastRect.height() };
    if (exportSz.cx <= 0 || exportSz.cy <= 0) {
        exportSz = { nrec.Width(), nrec.Height() };
    }

    if (cdc && hdc) {
        UINT pwFlag = Platform::IsWindows8Point1OrGreater() ? PW_RENDERFULLCONTENT : 0;
        void *pbmpl1{ nullptr };
        auto hb = _create32bitRGBDIB(nrec.Width(), -nrec.Height(), pbmpl1);
        if (hb) {
            auto hoj = SelectObject(cdc, hb);
            if (::PrintWindow(hWnd, cdc, pwFlag) == TRUE) {
                HDC ccdc = CreateCompatibleDC(cdc);
                void *pbmpl2{ nullptr };
                auto cchdib = _create32bitRGBDIB(exportSz.cx, -exportSz.cy, pbmpl2);
                if (ccdc && cchdib) {
                    auto cchobj = SelectObject(ccdc, cchdib);
                    CRect fr{ 0, 0, exportSz.cx, exportSz.cy };
                    FillRect(ccdc, fr, (HBRUSH)GetStockObject(WHITE_BRUSH));
                    SetStretchBltMode(ccdc, HALFTONE);
                    auto loffset = info.cxWindowBorders;
                    auto bRet = StretchBlt(ccdc, 0, 0, exportSz.cx, exportSz.cy, cdc, loffset, 0,
                                           nrec.Width() - loffset, nrec.Height(), SRCCOPY)
                        == TRUE;

                    _onCaptured((uint8_t*)pbmpl2, fr);
                    SelectObject(ccdc, cchobj);
                    DeleteObject(cchdib);
                    DeleteDC(ccdc);
                }
            }
            SelectObject(cdc, hoj);
            DeleteObject(hb);
        }
    }

    if (cdc)
        DeleteDC(cdc);
    if (hdc)
        ReleaseDC(hWnd, hdc);

    return 0;
}

bool DWMCapture::_createSession()
{
    BOOL enabled = FALSE;
    if (FAILED(DwmIsCompositionEnabled(&enabled)) || !enabled) {
        return -1;
    }

    const int x = GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int y = GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (!_hostWnd) {
        _hostWnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"", WS_POPUP | CS_DROPSHADOW, x, y, 10, 10, nullptr,
                                   nullptr, nullptr, nullptr);
    }

    HTHUMBNAIL thumbnail = nullptr;
    HRESULT hr = DwmRegisterThumbnail(_hostWnd, _hwnd, &thumbnail);
    if (SUCCEEDED(hr)) {
        SIZE size;
        hr = DwmQueryThumbnailSourceSize(thumbnail, &size);
        if (SUCCEEDED(hr)) {
            SetWindowPos(_hostWnd, nullptr, 0, 0, size.cx, size.cy,
                         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
            ShowWindow(_hostWnd, SW_SHOW);

            DWM_THUMBNAIL_PROPERTIES dskThumbProps;
            dskThumbProps.dwFlags
                = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE | DWM_TNP_SOURCECLIENTAREAONLY | DWM_TNP_OPACITY;
            dskThumbProps.fSourceClientAreaOnly = FALSE;
            dskThumbProps.fVisible = TRUE;
            dskThumbProps.opacity = 255;
            dskThumbProps.rcDestination = {0,0, size.cx, size.cy};
            hr = DwmUpdateThumbnailProperties(thumbnail, &dskThumbProps);
            if (SUCCEEDED(hr)) {
                _thumbnail = thumbnail;
                _lastThumbSize = size;
            }
        }
    }

    return _hostWnd != nullptr;
}

void DWMCapture::_updateThumbnailSize()
{
    if (!_thumbnail)
        return;

    SIZE size;
    HRESULT hr = DwmQueryThumbnailSourceSize(_thumbnail, &size);
    if (FAILED(hr))
        return;

    if (size.cx == _lastThumbSize.cx && size.cy == _lastThumbSize.cy)
        return;

    _lastThumbSize = size;

    SetWindowPos(_hostWnd, nullptr, 0, 0, size.cx, size.cy,
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    DWM_THUMBNAIL_PROPERTIES dskThumbProps;
    dskThumbProps.dwFlags = DWM_TNP_RECTDESTINATION;
    dskThumbProps.rcDestination = { 0, 0, size.cx, size.cy };
    DwmUpdateThumbnailProperties(_thumbnail, &dskThumbProps);
}

bool DWMCapture::_stopSession()
{
    DwmUnregisterThumbnail(_thumbnail);
    return false;
}
