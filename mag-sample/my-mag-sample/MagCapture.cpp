#include "stdafx.h"

#include <VersionHelpers.h>

#include "DesktopRect.h"
#include "MagCapture.h"

#include "CapUtility.h"
#include "logger.h"
#include "WinVersionHelper.h"


static wchar_t kMagnifierHostClass[] = L"HT-CapHostClass";
static wchar_t kHostWindowName[] = L"HT-CapHostWindow";
static wchar_t kMagnifierWindowClass[] = L"Magnifier"; // must be WC_MAGNIFIER
static wchar_t kMagnifierWindowName[] = L"HT-CapChildWindow";

DWORD GetTlsIndex()
{
    static DWORD tlsIndex = TlsAlloc();
    return tlsIndex;
}


MagCapture::MagCapture()
{
    _loadMagnificationAPI();
}


MagCapture::~MagCapture()
{
    stop();

    _api.reset(nullptr);

    if (_hMagModule)
        FreeLibrary(_hMagModule);
}

bool MagCapture::_loadMagnificationAPI()
{
    bool ret = false;

    _hMagModule = LoadLibraryW(L"magnification.dll");


    if (_hMagModule) {
        _api.reset(new MagInterface());

        _api->Initialize = reinterpret_cast<fnMagInitialize>(
            GetProcAddress(_hMagModule, "MagInitialize"));
        _api->Uninitialize = reinterpret_cast<fnMagUninitialize>(
            GetProcAddress(_hMagModule, "MagUninitialize"));
        _api->SetWindowFilterList = reinterpret_cast<fnMagSetWindowFilterList>(
            GetProcAddress(_hMagModule, "MagSetWindowFilterList"));
        _api->GetWindowFilterList = reinterpret_cast<fnMagGetWindowFilterList>(
            GetProcAddress(_hMagModule, "MagGetWindowFilterList"));
        _api->SetWindowSource = reinterpret_cast<fnMagSetWindowSource>(
            GetProcAddress(_hMagModule, "MagSetWindowSource"));
        _api->GetWindowSource = reinterpret_cast<fnMagGetWindowSource>(
            GetProcAddress(_hMagModule, "MagGetWindowSource"));
        _api->SetImageScalingCallback = reinterpret_cast<fnMagSetImageScalingCallback>(
            GetProcAddress(_hMagModule, "MagSetImageScalingCallback"));
        _api->SetColorEffect = reinterpret_cast<fnMagSetColorEffect>(
            GetProcAddress(_hMagModule, "MagSetColorEffect"));
    }

    ret = !(!_hMagModule
        || !_api->Initialize
        || !_api->Uninitialize
        || !_api->SetWindowFilterList
        || !_api->GetWindowFilterList
        || !_api->SetWindowSource
        || !_api->GetWindowSource
        || !_api->SetImageScalingCallback);

    return ret;
}


bool MagCapture::_initMagnifier(DesktopRect &rect)
{
    BOOL result = TRUE;
    HMODULE hInstance = nullptr;
    const char *info = "OK";

    do 
    {
        /*
         * It will not work when DWM disabled.
         */
        if (Platform::IsWindows7OrGreater()) {
            DwmIsCompositionEnabled(&result);
        }
        if (!result) {
            info = "DWM disabled.";
            break;
        }

        result = _api->Initialize();
        if (!result) {
            info = "Mag Init.";
            break;
        }

        result = 
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                 reinterpret_cast<char *>(&DefWindowProc), &hInstance);
        if (!result) {
            info = "Get Module handle.";
            break;
        }

        // Register the host window class. See the MSDN documentation of the
        // Magnification API for more information.
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.lpfnWndProc = &DefWindowProc;
        wcex.hInstance = hInstance;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.lpszClassName = kMagnifierHostClass;
        wcex.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;

        // Ignore the error which may happen when the class is already registered.
        RegisterClassExW(&wcex);

        // Create the host window.
        _hostWnd = CreateWindowExW(WS_EX_TOPMOST |WS_EX_LAYERED, kMagnifierHostClass, kHostWindowName,
                                   WS_CLIPCHILDREN | WS_POPUP | WS_EX_TRANSPARENT | // Click-through
                                       WS_EX_TOOLWINDOW,                            // Do not show program on taskbar,
                                   0, 0, rect.width(), rect.height(), nullptr, nullptr, hInstance,
                                   nullptr);
        if (!_hostWnd) {
            info = "Create Mag Host Window.";
            break;
        }

        SetLayeredWindowAttributes(_hostWnd, 0, 255, LWA_ALPHA);

        // Create the magnifier control.
        _magWnd = CreateWindowExW(0, kMagnifierWindowClass, kMagnifierWindowName, WS_CHILD | MS_SHOWMAGNIFIEDCURSOR, 0,
                                  0, rect.width(), rect.height(), _hostWnd, nullptr, hInstance, nullptr);
        if (!_magWnd) {
            info = "Create Mag Window.";
            break;
        }

        // Hide the host window.
        ShowWindow(_hostWnd, SW_HIDE);

        // Set the scaling callback to receive captured image.
        result = _api->SetImageScalingCallback(_magWnd, &MagCapture::_OnMagImageScalingCallback);
        if (!result) {
            info = "Set Image Scaling Callback.";
            break;
        }

    } while (0);
    
    if (!result) {
        logger::logError("init magnifier failed %s", info);
        _destoryMagnifier();
    }
    else {
        _hMagInstance = hInstance;
    }

    return result;
}

bool MagCapture::_destoryMagnifier()
{
    bool ret = false;

    if (_hostWnd) {
        ::SendMessageW(_hostWnd, WM_CLOSE, 0, 0);
        ::DestroyWindow(_hostWnd);
        UnregisterClassW(kMagnifierHostClass, _hMagInstance);
    }

    _magWnd = nullptr;
    _hostWnd = nullptr;

    if (_api->Uninitialize)
        _api->Uninitialize();

    return ret;
}


bool MagCapture::onCaptured(void *srcdata, MAGIMAGEHEADER header)
{
    int width = _lastRect.width();
    int height = _lastRect.height();
    int stride = width * 4;
    auto &inStride = header.stride;

    if ( header.stride < stride )
    {
        return false;
    }

    /*
     * on multiple screen platform, offset maybe changed and may cause crash, so disable one.
     */
    if (_offset != -1 && header.offset != _offset) {
        return false;
    }

    //logger::log(LogLevel::Info, " %dx%d,%d, %d, %d\n", header.width, header.height, header.offset, header.stride, header.cbSize);

    int bpp = header.cbSize / header.width / header.height; // bpp should be 4
    if (!_frames.get() || header.format != GUID_WICPixelFormat32bppRGBA 
        || width != _frames->width() || height != _frames->height()
        || stride != _frames->stride() || bpp != CapUtility::kDesktopCaptureBPP) {
        _frames.reset(VideoFrame::MakeFrame(width, height, stride,
                                            VideoFrameType::kVideoFrameTypeRGBA));
    }

    {
        uint8_t *pDst = _frames->data();
        uint8_t *pSrc = reinterpret_cast<uint8_t *>(srcdata) + header.offset;

        for (int i = 0; i < height; i++) {
            memcpy(pDst, pSrc, stride);
            pDst += stride;
            pSrc += inStride;
        }
    }

    {
        std::lock_guard<decltype(_cbMutex)> guard(_cbMutex);

        if (_callback) {
            _callback(_frames.get(), _callbackargs);
        }
    }

    _offset = header.offset;

    return true;
}

BOOL WINAPI MagCapture::_OnMagImageScalingCallback(HWND hwnd,
    void* srcdata,
    MAGIMAGEHEADER srcheader,
    void* destdata,
    MAGIMAGEHEADER destheader,
    RECT unclipped,
    RECT clipped,
    HRGN dirty)
{
    BOOL bRet = FALSE;
    MagCapture *owner = reinterpret_cast<MagCapture *>(TlsGetValue(GetTlsIndex()));
    
    if (owner)
        bRet = owner->onCaptured(srcdata, srcheader);

    return bRet;
}


bool MagCapture::setCallback(funcCaptureCallback fcb, void* args)
{
    {
        std::lock_guard<decltype(_cbMutex)> guard(_cbMutex);
        _callback = fcb;
        _callbackargs = args;
    }
    return true;
}


bool MagCapture::captureImage(const DesktopRect& capRect) 
{
    bool bRet = false;
    DesktopRect rect = capRect;

    __try {
        if (!rect.is_empty()) {
            RECT wRect = { rect.left(), rect.top(), rect.right(), rect.bottom() };

            _lastRect = rect;
            TlsSetValue(GetTlsIndex(), this);

            // OnCaptured will be called via OnMagImageScalingCallback and fill in the
            // frame before set_window_source_func_ returns.
            bRet = _api->SetWindowSource(_magWnd, wRect) == TRUE;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        bRet = false;
        logger::logInfo("exception");
    }

    return bRet;
}


bool MagCapture::setExcludeWindows(std::vector<HWND>& hWnd)
{
    bool ret = false;

    ret = _api->SetWindowFilterList(_magWnd, MW_FILTERMODE_EXCLUDE, hWnd.size(), hWnd.data()) == TRUE;

    return ret;
}


bool MagCapture::startCaptureWindow(HWND hWnd)
{
    bool ret = false;

    HMONITOR hm = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    CapUtility::DisplaySetting settings = CapUtility::enumDisplaySettingByMonitor(hm);
    DesktopRect rect = DesktopRect::MakeRECT(settings.rect());

    RECT primeryRect;
    CapUtility::GetPrimeryWindowsRect(primeryRect);

    ret = _initMagnifier(rect);

    return ret;
}


bool MagCapture::startCaptureScreen(HMONITOR hMonitor)
{
    bool ret = false;
    CapUtility::DisplaySetting settings = CapUtility::enumDisplaySettingByMonitor(hMonitor);
    DesktopRect rect = DesktopRect::MakeRECT(settings.rect());

    RECT primeryRect;
    CapUtility::GetPrimeryWindowsRect(primeryRect);

    ret = _initMagnifier(rect);

    return ret;
}

bool MagCapture::stop()
{
    bool ret = false;

    _destoryMagnifier();

    return ret;
}

const char *MagCapture::getName()
{
    return "MAG capture";
}

bool MagCapture::usingTimer()
{
    return true;
}