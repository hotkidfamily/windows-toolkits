// DXPCIeDetector.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <memory>
#include <thread>

#include <d3d11.h>
#include <wrl/client.h>
#include "CPGPU.h"
#include "SlidingWindow.h"

using Microsoft::WRL::ComPtr;

float _fps = 0;
bool _exit_test = false;

typedef struct Context
{
    int width;
    int height;
    int stride;
    int bits;
    int bytesPerFrame;
    PixelFormat format;
} Context;

Context ctx{ 3840, 2160, 3840, 8, 3840* 2160, PixelFormat_GRAY };

int proc() 
{
    std::unique_ptr<utils::SlidingWindow> bench = std::make_unique<utils::SlidingWindow>(3000);
    const wchar_t* _errorInfo = L"no error";

    std::unique_ptr<CPGPU> device = std::make_unique<CPGPU>();
    auto hr = device->createDevice();
    if (FAILED(hr))
    {
        return -1;
    }

    DXGI_FORMAT src_format = DXGI_FORMAT_R8_UNORM;
    uint8_t* rawdata = (uint8_t*)_aligned_malloc(ctx.bytesPerFrame, 1024);
    uint8_t* res = (uint8_t*)_aligned_malloc(ctx.width * ctx.height * (ctx.bits > 8 ? 2 : 1), 1024);

    GraphFrame frame{ ctx.width, ctx.height, ctx.bits, ctx.stride, ctx.format, { rawdata }, { nullptr } };

    ComPtr<ID3D11Texture2D> rawStage;
    auto dev = device->getDevice();
    hr = CPGPU::MakeTex(dev, ctx.stride, ctx.height, src_format, &rawStage, D3D11_USAGE_STAGING, 0,
        D3D11_CPU_ACCESS_WRITE);
    if (FAILED(hr))
    {
        _errorInfo = L"Error: No gpu memory";
        return -3;
    }

    ComPtr<ID3D11Texture2D> srcText;
    ComPtr<ID3D11ShaderResourceView> srcTextSRV;
    hr = CPGPU::MakeTexAndSRV(dev, ctx.stride, ctx.height, src_format, &srcText, &srcTextSRV, D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE, 0);
    if (FAILED(hr))
    {
        _errorInfo = L"Error: no gpu memory";
        return -3;
    }
    frame.gpumem.text = srcText;
    frame.gpumem.srv = srcTextSRV;

    D3D11_TEXTURE2D_DESC desc{ 0 };
    srcText->GetDesc(&desc);
    ComPtr<ID3D11Texture2D> outStage;
    hr = CPGPU::MakeTex(dev, desc.Width, desc.Height, desc.Format, &outStage, D3D11_USAGE_STAGING, 0,
        D3D11_CPU_ACCESS_READ);
    if (FAILED(hr))
    {
        _errorInfo = L"Error: No gpu memory";
        return -3;
    }

    device->upload(rawStage.Get(), &frame);

    do
    {
        GraphFrame input{ 0 };
        input = frame;
        device->copy(frame.gpumem.text.Get(), rawStage.Get());
        bench->append();

        input.cpumem.data = res;
        input.gpumem.textStage = outStage;
        //device->copy(input.gpumem.text.Get(), frame.gpumem.text.Get());
        //bench->append();

        device->copy(input.gpumem.textStage.Get(), input.gpumem.text.Get());
        bench->append();
        device->flush();

        _fps = bench->fps();

        //device->download(&input);
    } while (!_exit_test);


    if (rawdata) {
        _aligned_free(rawdata);
    }

    if (res) {
        _aligned_free(res);
    }

    return 0;
}

BOOL WINAPI CtrlHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT)
    {
        _exit_test = true;
        return TRUE;
    }
    return FALSE; // 返回 FALSE 表示未处理
}

int main()
{
    printf("----> Start!!!!!\n");
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    std::thread t = std::thread(&proc);
    char buff[2014];

    while (!_exit_test) {
        auto fps = _fps;
        //std::cout << "\r" << fps << "[" << ctx.bytesPerFrame * fps / 1000 / 1000 / 1000 << "]" << std::endl;
        snprintf(buff, 2014, "%.2f fps / [%.2f] GB     ", fps, ctx.bytesPerFrame * fps / 1000 / 1000 / 1000);
        printf("\r ---> %s", buff);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    if (t.joinable()) {
        t.join();
    }

    printf("\n----> End!!!!!\n");
}
