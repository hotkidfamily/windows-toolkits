#include "stdafx.h"
#include "WGCCapture.h"

#include <functional>

#include "CapUtility.h"
#include "CPGPU.h"

#include "logger.h"
#include "WinVersionHelper.h"
#include <winrt/windows.foundation.metadata.h>

#pragma comment(lib, "windowsapp.lib")

using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics;

#define WM_STARTCPT_MSGE (WM_USER + 1)
#define WM_STOPCPT_MSGE  (WM_USER + 2)


WGCCapture::WGCCapture()
{
    _thread = std::thread(std::bind(&WGCCapture::_run, this));
    _threadID = GetThreadId(_thread.native_handle());
}

WGCCapture::~WGCCapture()
{
    PostThreadMessage(_threadID, WM_QUIT, 0, 0);
    if (_thread.joinable()) {
        _thread.join();
    }
}

bool WGCCapture::_createSession()
{
    const auto activationFactory = winrt::get_activation_factory<GraphicsCaptureItem>();
    auto interopFactory = activationFactory.as<IGraphicsCaptureItemInterop>();
    GraphicsCaptureItem cptItem = { nullptr };

    try {
        if (_hwnd != nullptr) {
            winrt::check_hresult(interopFactory->CreateForWindow(
                _hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
                winrt::put_abi(cptItem)));
        }
        else {
            winrt::check_hresult(interopFactory->CreateForMonitor(
                _hmonitor, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
                winrt::put_abi(cptItem)));
        }
    }
    catch (winrt::hresult_error e) {
        logger::logErrorW(L"WGC: %s [%x][%x]", e.message().c_str(), _hwnd, _hmonitor);
        return false;
    }

    auto sz = cptItem.Size();
    auto framePool = Direct3D11CaptureFramePool::Create(
        _d3dDevice, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, sz);
    auto session = framePool.CreateCaptureSession(cptItem);
    auto revoker = framePool.FrameArrived(winrt::auto_revoke, { this, &WGCCapture::_onFrameArrived });

    if (Platform::IsWin10_2004OrGreater()) {
        //if (Windows.Foundation.Metadata.ApiInformation.IsApiContractPresent(
        // "Windows.Foundation.UniversalApiContract", 10)) // 2004
        if (winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
                L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsCursorCaptureEnabled")) {
            session.IsCursorCaptureEnabled(false);
        }
    }

    if (Platform::IsWin10_21H1OrGreater())
    // if (Windows.Foundation.Metadata.ApiInformation.IsApiContractPresent(
    // "Windows.Foundation.UniversalApiContract", 12)) // 2104
    if (winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
            L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsBorderRequired")){
        session.IsBorderRequired(false);
    }

    if (Platform::IsWin11_24H2OrGreater()) {
        // if (Windows.Foundation.Metadata.ApiInformation.IsApiContractPresent(
        // "Windows.Foundation.UniversalApiContract", 19)) // 2104
        if (winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
                L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IncludeSecondaryWindows")) {
            session.IncludeSecondaryWindows(true);
        }
        if (winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
                L"Windows.Graphics.Capture.GraphicsCaptureSession", L"MinUpdateInterval")) {
            session.MinUpdateInterval(std::chrono::milliseconds(10));
        }
    }
        
    session.StartCapture();

    _session = session;
    _framePool = framePool;
    _item = cptItem;
    _frame_arrived_revoker = std::move(revoker);
    _curFramePoolSz = sz;

    return true;
}

bool WGCCapture::_stopSession()
{
    SetWindowLong(_hwnd, GWL_EXSTYLE, GetWindowLong(_hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    _frame_arrived_revoker.revoke();
    if (_framePool)
        _framePool.Close();
    if (_session)
        _session.Close();

    _framePool = nullptr;
    _session = nullptr;
    _item = nullptr;

    return false;
}

bool WGCCapture::startCaptureWindow(HWND hWnd)
{
    _hwnd = hWnd;
    _hmonitor = nullptr;

    if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {
        return false;
    }

    auto ret = PostThreadMessage(_threadID, WM_STARTCPT_MSGE, 0, 0);
    if (!ret) {
        auto gle = GetLastError();
    }

    return true;
}

bool WGCCapture::startCaptureScreen(HMONITOR hMonitor)
{
    _hwnd = nullptr;
    _hmonitor = hMonitor;

    if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {
        return false;
    }
    auto ret = PostThreadMessage(_threadID, WM_STARTCPT_MSGE, 0, 0);
    if (!ret) {
        auto gle = GetLastError();
    }

    return true;
}

bool WGCCapture::stop()
{
    auto ret = PostThreadMessage(_threadID, WM_STOPCPT_MSGE, 0, 0);
    if (!ret) {
        auto gle = GetLastError();
    }
    return true;
}

bool WGCCapture::captureImage(const DesktopRect &rect)
{
    _lastRect = rect;
    return true;
}

bool WGCCapture::setCallback(funcCaptureCallback fcb, void *args)
{
    std::lock_guard<decltype(_cbMutex)> guard(_cbMutex);
    _callback = fcb;
    _callbackargs = args;

    return false;
}

bool WGCCapture::setExcludeWindows(std::vector<HWND> &hWnd)
{
    return false;
}

bool WGCCapture::_onCaptured(D3D11_MAPPED_SUBRESOURCE &rect, D3D11_TEXTURE2D_DESC &header)
{
    bool bRet = false;
    int x = abs(_lastRect.left());
    int y = abs(_lastRect.top());
    int width = _lastRect.width();
    int height = _lastRect.height();
    int stride = width * 4;
    auto inStride = rect.RowPitch;

    if (_lastRect.is_empty()) {
        width = header.Width;
        height = header.Height;
        stride = width * 4;
    }

    uint8_t *pBits = (uint8_t *)rect.pData;
    DesktopRect capRect = DesktopRect::MakeXYWH(0, 0, header.Width, header.Height);

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

    {
        std::lock_guard<decltype(_cbMutex)> guard(_cbMutex);

        if (_callback) {
            _callback(_frames.get(), _callbackargs);
        }
    }

    return bRet;
}


void WGCCapture::_onFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const &sender,
                                 winrt::Windows::Foundation::IInspectable const &handler)
{
    winrt::com_ptr<ID3D11Texture2D> texture;
    auto frame = sender.TryGetNextFrame();
    auto contentSz = frame.ContentSize();

    if (_curFramePoolSz.Width != contentSz.Width || _curFramePoolSz.Height != contentSz.Height) {
        _curFramePoolSz = contentSz;
        _framePool.Recreate(_d3dDevice, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, contentSz);
    }

    auto access = frame.Surface().as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void());

    winrt::com_ptr<ID3D11DeviceContext> ctx;
    _d3d11device->GetImmediateContext(ctx.put());
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);

    // desc.Usage = D3D11_USAGE_STAGING;
    // desc.BindFlags = 0;
    // desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    // desc.MiscFlags = 0;

    // winrt::com_ptr<ID3D11Texture2D> userTexture = nullptr;
    // winrt::check_hresult(_dev->CreateTexture2D(&desc, NULL, userTexture.put()));

    if (!_tempTexture) {
        winrt::check_hresult(CPGPU::MakeTex(_d3d11device.get(), desc.Width, desc.Height, desc.Format,
                                            _tempTexture.put(), D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ));
    }

    D3D11_TEXTURE2D_DESC desc2;
    _tempTexture->GetDesc(&desc2);
    if (desc2.Width != desc.Width || desc2.Height != desc.Height) {
        winrt::check_hresult(CPGPU::MakeTex(_d3d11device.get(), desc.Width, desc.Height, desc.Format,
                                            _tempTexture.put(), D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ));
    }

    ctx->CopyResource(_tempTexture.get(), texture.get());
    D3D11_MAPPED_SUBRESOURCE resource;
    if(ctx->Map(_tempTexture.get(), NULL, D3D11_MAP_READ, 0, &resource) == S_OK)
    {
        D3D11_TEXTURE2D_DESC desc{ 0 };
        _tempTexture->GetDesc(&desc);
        _onCaptured(resource, desc);
        ctx->Unmap(_tempTexture.get(), 0);
    }

    return;
}

void WGCCapture::_run()
{
    MSG msg{};
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    winrt::init_apartment(winrt::apartment_type::multi_threaded);

    DispatcherQueueOptions options{ sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_STA };
    winrt::com_ptr<winrt::Windows::System::IDispatcherQueueController> dqController{ nullptr };
    winrt::check_hresult(CreateDispatcherQueueController(
        options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController **>(dqController.put())));
    winrt::Windows::System::DispatcherQueue* queue{ nullptr };
    dqController->get_DispatcherQueue(reinterpret_cast<void**>(&queue));

    winrt::com_ptr<ID3D11Device> d3dDevice;
    winrt::check_hresult(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                           nullptr, 0, D3D11_SDK_VERSION, d3dDevice.put(), nullptr, nullptr));
    winrt::com_ptr<IDXGIDevice> dxgiDevice = d3dDevice.as<IDXGIDevice>();
    winrt::com_ptr<IInspectable> inspectable;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectable.put()));
    auto device = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

    _d3d11device = std::move(d3dDevice);
    _d3dDevice = std::move(device);
    _queueController = std::move(dqController);
    _queue = queue;

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_STARTCPT_MSGE) {
            _stopSession();
            _createSession();
        }
        else if (msg.message == WM_STOPCPT_MSGE) {
            _stopSession();
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    _stopSession();

    winrt::uninit_apartment();
}
