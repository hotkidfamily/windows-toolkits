#include "stdafx.h"
#include "DXGICapture.h"
#include "CapUtility.h"

#include "logger.h"

DXGICapture::DXGICapture()
{
    _loadD3D11();
}

DXGICapture::~DXGICapture()
{
    stop();
    _deinit();
}

bool DXGICapture::_loadD3D11()
{
    bool bRet = false;

    HMODULE hm = LoadLibraryW(L"D3D11.dll");
    if (hm) {
        fnD3D11CreateDevice = reinterpret_cast<decltype(fnD3D11CreateDevice)>(GetProcAddress(hm, "D3D11CreateDevice"));
    }

    HMODULE hm2 = LoadLibraryW(L"DXGI.dll");
    if (hm2) {
        fnCreateDXGIFactory1
            = reinterpret_cast<decltype(fnCreateDXGIFactory1)>(GetProcAddress(hm2, "CreateDXGIFactory1"));
    }

    bRet = !!hm && !!fnD3D11CreateDevice && !!hm2 && !! fnCreateDXGIFactory1;

    return bRet;
}

bool DXGICapture::_init(HMONITOR &hm)
{
    bool bRet = false;
    HRESULT hr = E_FAIL;

    do {
        if (!fnD3D11CreateDevice) {
            break;
        }

        ComPtr<IDXGIFactory1> factory;
        HRESULT err = fnCreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&factory);
        ComPtr<IDXGIAdapter1> hDxgiAdapter = NULL;
        UINT n = 0;
        bool bFound = false;
        ComPtr<IDXGIOutput> hDxgiOutput = NULL;
        while (factory->EnumAdapters1(n, &hDxgiAdapter) == S_OK) {
            UINT nOutput = 0;
            do {
                hr = hDxgiAdapter->EnumOutputs(nOutput, &hDxgiOutput);
                if (hr == DXGI_ERROR_NOT_FOUND) {
                    break;
                }

                DXGI_OUTPUT_DESC eDesc = {};
                hDxgiOutput->GetDesc(&eDesc);
                if (eDesc.Monitor == hm) {
                    bFound = true;
                    break;
                }
                ++nOutput;
            } while (SUCCEEDED(hr));

            if (bFound) {
                break;
            }
            n++;
        }

        D3D_DRIVER_TYPE DriverTypes[] = {
            D3D_DRIVER_TYPE_UNKNOWN,
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE, D3D_DRIVER_TYPE_SOFTWARE,
        };
        UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

        D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                              D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_1 };
        UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

        D3D_FEATURE_LEVEL FeatureLevel;

        for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex) {
            hr = fnD3D11CreateDevice(hDxgiAdapter.Get(), DriverTypes[DriverTypeIndex], NULL, 0, FeatureLevels,
                                     NumFeatureLevels, D3D11_SDK_VERSION, &_dev, &FeatureLevel, &_ctx);
            if (SUCCEEDED(hr)) {
                break;
            }
        }
        if (FAILED(hr)) {
            break;
        }

        hDxgiOutput->GetDesc(&_outputDesc);

        ComPtr<IDXGIOutput1> hDxgiOutput1 = NULL;
        hr = hDxgiOutput.As(&hDxgiOutput1);
        if (FAILED(hr)) {
            break;
        }

        hr = hDxgiOutput1->DuplicateOutput(_dev.Get(), &_desktopDuplication);
        if (FAILED(hr)) {
            break;
        }
    } while (0);

    bRet = SUCCEEDED(hr);

    return bRet;
}

void DXGICapture::_deinit()
{
}

bool DXGICapture::_onCaptured(DXGI_MAPPED_RECT &rect, DXGI_OUTPUT_DESC &header)
{
    int width = _lastRect.width();
    int height = _lastRect.height();
    int stride = width * 4;
    auto inStride = rect.Pitch;

    uint8_t *pBits = (uint8_t *)rect.pBits;

    int bpp = CapUtility::kDesktopCaptureBPP;
    if (!_frames.get() || width != static_cast<UINT>(_frames->width()) || height != static_cast<UINT>(_frames->height())
        || stride != static_cast<UINT>(_frames->stride()) || bpp != CapUtility::kDesktopCaptureBPP) {
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

    return true;
}

bool DXGICapture::startCaptureWindow(HWND hWnd)
{
    return false;
}

bool DXGICapture::startCaptureScreen(HMONITOR hMonitor)
{
    if (!_init(hMonitor))
        return false;

    startLoop();
    return true;
}

bool DXGICapture::stop()
{
    stopLoop();
    return true;
}

void DXGICapture::onCaptureLoop()
{
    DesktopRect rect = getCaptureRect();
    _captureImage(rect);
}

bool DXGICapture::_captureImage(const DesktopRect &rect)
{
    bool bRet = false;

    _lastRect = rect;

    ComPtr<IDXGIResource> hDesktopResource = NULL;
    DXGI_OUTDUPL_FRAME_INFO FrameInfo;
    HRESULT hr = _desktopDuplication->AcquireNextFrame(0, &FrameInfo, &hDesktopResource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return TRUE;
    }
    else if (FAILED(hr)) {
        return TRUE;
    }

    ComPtr<ID3D11Texture2D> hAcquiredDesktopImage = NULL;
    hr = hDesktopResource.As(&hAcquiredDesktopImage);
    if (FAILED(hr)) {
        return FALSE;
    }

    D3D11_TEXTURE2D_DESC fDesc = {0};
    hAcquiredDesktopImage->GetDesc(&fDesc);
    if (memcmp(&_sourceFormat, &fDesc, sizeof(D3D11_TEXTURE2D_DESC)) != 0) {
        _sourceFormat = fDesc;
        fDesc.Usage = D3D11_USAGE_STAGING;
        fDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        fDesc.BindFlags = 0;
        fDesc.MiscFlags = 0;
        fDesc.MipLevels = 1;
        fDesc.ArraySize = 1;
        fDesc.SampleDesc.Count = 1;
        hr = _dev->CreateTexture2D(&fDesc, NULL, &_destFrame);
        if (FAILED(hr)) {
            _desktopDuplication->ReleaseFrame();
            return FALSE;
        }
    }

    _ctx->CopyResource(_destFrame.Get(), hAcquiredDesktopImage.Get());
    _desktopDuplication->ReleaseFrame();

    ComPtr<IDXGISurface1> hStagingSurf = NULL;
    hr = _destFrame.As(&hStagingSurf);
    if (FAILED(hr)) {
        return FALSE;
    }

    DXGI_MAPPED_RECT mappedRect;
    hr = hStagingSurf->Map(&mappedRect, DXGI_MAP_READ);
    if (SUCCEEDED(hr)) {
        _onCaptured(mappedRect, _outputDesc);
        hStagingSurf->Unmap();
    }

    bRet = SUCCEEDED(hr);

    return bRet;
}

const char *DXGICapture::getName()
{
    return "DXGI Capture";
}
