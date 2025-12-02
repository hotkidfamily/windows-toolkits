// memcpy_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#include <windows.h>
#include <wrl/client.h>
#include <d3d11.h>

#include <assert.h>

using Microsoft::WRL::ComPtr;


#pragma comment(lib,  "d3d11.lib")

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);

struct app_context
{
    struct {
        bool running = false;
        int64_t seconds = 0;
    }machine;

    struct {
        int32_t threads = 0;
        int64_t counter = 0;
        int32_t batch_size = 1;
        int32_t fps = -1;
    }cpu;

    struct {
        int32_t threads = 0;
        int64_t counter = 0;
        int32_t batch_size = 1;
        int32_t fps = -1;
    }gpu;

    std::vector<std::thread> thread_objs;
};

app_context context;

void memcpy_thread(app_context* ctx)
{
    struct command {
        void* src = 0;
        void* dst = 0;
        int64_t length = 0;
    } cmd;

    cmd.length = ctx->cpu.batch_size * 1024 * 1024;
    cmd.src = _aligned_malloc(cmd.length, 32);
    cmd.dst = _aligned_malloc(cmd.length, 32);

    int32_t interval = ctx->cpu.fps > 0 ? ctx->cpu.fps : -1;

    std::cout << "[cpu] excute in " << interval << " ms" << std::endl;

    while (ctx->machine.running) {
        memcpy(cmd.dst, cmd.src, cmd.length);
        InterlockedIncrement64(&ctx->cpu.counter);
        if (interval > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    }

    _aligned_free(cmd.src);
    _aligned_free(cmd.dst);
}

void mem_map_thread(app_context* ctx)
{
    D3D_DRIVER_TYPE type = D3D_DRIVER_TYPE_HARDWARE;
    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    D3D_FEATURE_LEVEL using_level = D3D_FEATURE_LEVEL_12_0;
#if _debug
    auto dev_flag = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG;
#else
    auto dev_flag = D3D11_CREATE_DEVICE_SINGLETHREADED;
#endif
    HRESULT hret = D3D11CreateDevice(nullptr, type, nullptr, dev_flag,
        levels, _countof(levels), D3D11_SDK_VERSION, &device, &using_level, &context);
    if (SUCCEEDED(hret)) {

    }

    D3D11_TEXTURE2D_DESC desc = { 0 };
    desc.Width = 1920;
    desc.Height = 1080;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> tex;
    hret = device->CreateTexture2D(&desc, nullptr, tex.GetAddressOf());
    assert(SUCCEEDED(hret));

    int32_t len = ctx->gpu.batch_size * 1024 * 1024;
    int32_t gpu_caps = 1920 * 1080 * 4;
    int8_t* sys_src = (int8_t*)_aligned_malloc(len, 32);

    int32_t interval = ctx->cpu.fps > 0 ? ctx->cpu.fps : -1;

    std::cout << "[gpu] excute in " << interval << " ms" << std::endl;

    while ( ctx->machine.running) {
        D3D11_MAPPED_SUBRESOURCE s{ 0 };
        hret = context->Map(tex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &s);
        assert(SUCCEEDED(hret));

        if(gpu_caps > len){
            memcpy(s.pData, sys_src, len);
        }
        else {
            int8_t* dst = reinterpret_cast<int8_t*>(s.pData);
            auto src = sys_src;
            for (int i = 0; i < 1080; i++) {
                memcpy(dst, src, 1920);
                dst += s.RowPitch;
                src += 1920;
            }
        }

        if (SUCCEEDED(hret)) {
            context->Unmap(tex.Get(), 0);
        }

        InterlockedIncrement64(&ctx->gpu.counter);

        if (interval > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    }


    if (sys_src) {
        _aligned_free(sys_src);
    }
}

int main(int argc,  char *argv[])
{
    bool bret = SetConsoleCtrlHandler(CtrlHandler, TRUE);
    assert(bret);
    std::cout << "usage: "<< argv[0] << " threads[cpu] MB[cpu] threads[gpu] MB[gpu] fps" << std::endl;

    if (argc >= 2) {
        context.cpu.threads = atoi(argv[1]);
    }

    if (argc >= 3) {
        context.cpu.batch_size = atoi(argv[2]);
    }

    if (argc >= 4) {
        context.gpu.threads = atoi(argv[3]);
    }

    if (argc >= 5) {
        context.gpu.batch_size = atoi(argv[4]);
    }

    if (argc >= 6) {
        context.cpu.fps = context.gpu.fps = 1000 / atoi(argv[5]);
    }

    context.machine.running = true;

    std::cout << "run benchmark for \n" 
        << " [cpu] " << context.cpu.batch_size << " MB with " << context.cpu.threads << " threads and " << 1000 / context.cpu.fps << " fps \n"
        << " [gpu] " << context.gpu.batch_size << " MB with " << context.gpu.threads << " threads and " << 1000 / context.gpu.fps << " fps \n"
        << std::endl;

    for (auto i = 0; i < context.cpu.threads; i++) {
        std::thread td(memcpy_thread, &context);
        context.thread_objs.push_back(std::move(td));
    }

    for (auto i = 0; i < context.gpu.threads; i++) {
        std::thread td(mem_map_thread, &context);
        context.thread_objs.push_back(std::move(td));
    }

    while( context.machine.running ){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto v = context.cpu.counter;
        auto len = v * context.cpu.batch_size;
        auto len_gb = len / 1024;

        auto c_gpu = context.gpu.counter;
        auto len_g = c_gpu * context.gpu.batch_size;
        auto len_gb_g = len_g / 1024;

        context.machine.seconds++;
        auto sec = context.machine.seconds;
        std::cout << "excute in(" << sec << "): cpu " << v << " times " << v / sec << "fps, bandwidth: total " << len << " MB,"
            << len_gb << " GB " << len_gb / sec << " GB/s "
            << "\n        gpu " << c_gpu << " times " << c_gpu / sec << "fps, bandwidth: total " << len_g <<" MB,"
            << len_gb_g << " GB " << len_gb_g / sec << " GB/s"
            << std::endl;
    }

    for (auto &obj : context.thread_objs){
        if (obj.joinable())
            obj.join();
    }

    SetConsoleCtrlHandler(CtrlHandler, FALSE);
    return 0;
}


BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    context.machine.running = false;
    const char* info;

    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
        info = "Ctrl-C event";
        std::cout << info << std::endl;
        return TRUE;

    case CTRL_CLOSE_EVENT:
        info = "Ctrl-Close event";
        std::cout << info << std::endl;
        return TRUE;

    case CTRL_BREAK_EVENT:
        info = "Ctrl-Break event";
        std::cout << info << std::endl;
        return FALSE;

    case CTRL_LOGOFF_EVENT:
        info = "Ctrl-Logoff event";
        std::cout << info << std::endl;
        return FALSE;

    case CTRL_SHUTDOWN_EVENT:
        info = "Ctrl-Shutdown event";
        std::cout << info << std::endl;
        return FALSE;

    default:
        info = "unsupport event";
        std::cout << info << std::endl;
        return FALSE;
    }

}
