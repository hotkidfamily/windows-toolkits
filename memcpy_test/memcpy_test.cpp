// memcpy_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <string>

#include <windows.h>
#include <wrl/client.h>
#include <d3d11.h>

#include <assert.h>
#include <immintrin.h>

using Microsoft::WRL::ComPtr;


#pragma comment(lib,  "d3d11.lib")

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);

struct app_context
{
    struct {
        std::atomic<bool> running = false;
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

void stream_copy(void* dst, const void* src, size_t size)
{
    auto d = reinterpret_cast<__m256i*>(dst);
    auto s = reinterpret_cast<const __m256i*>(src);
    size_t chunks = size / 32;
    size_t remainder = size % 32;

    for (size_t i = 0; i < chunks; i++) {
        __m256i data = _mm256_load_si256(s + i);
        _mm256_stream_si256(d + i, data);
    }

    if (remainder > 0) {
        memcpy(reinterpret_cast<int8_t*>(d + chunks),
               reinterpret_cast<const int8_t*>(s + chunks), remainder);
    }

    _mm_sfence();
}

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
        stream_copy(cmd.dst, cmd.src, cmd.length);
        InterlockedAdd64(&ctx->cpu.counter, ctx->cpu.batch_size);
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
#if _DEBUG
    auto dev_flag = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG;
#else
    auto dev_flag = D3D11_CREATE_DEVICE_SINGLETHREADED;
#endif
    HRESULT hret = D3D11CreateDevice(nullptr, type, nullptr, dev_flag,
        levels, _countof(levels), D3D11_SDK_VERSION, &device, &using_level, &context);
    if (FAILED(hret)) {
        std::cout << "[gpu] D3D11CreateDevice failed: 0x" << std::hex << hret << std::dec << std::endl;
        return;
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
    int32_t row_bytes = 1920 * 4;
    int32_t gpu_caps = row_bytes * 1080;
    int32_t actual_copy = len < gpu_caps ? len : gpu_caps;
    int8_t* sys_src = (int8_t*)_aligned_malloc(len, 32);

    int32_t interval = ctx->gpu.fps > 0 ? ctx->gpu.fps : -1;

    std::cout << "[gpu] excute in " << interval << " ms" << std::endl;

    while ( ctx->machine.running) {
        D3D11_MAPPED_SUBRESOURCE s{ 0 };
        hret = context->Map(tex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &s);
        assert(SUCCEEDED(hret));

        if(gpu_caps > len){
            int8_t* dst = reinterpret_cast<int8_t*>(s.pData);
            auto src = sys_src;
            int32_t remaining = len;
            for (int i = 0; i < 1080 && remaining > 0; i++) {
                int32_t copy_len = remaining < row_bytes ? remaining : row_bytes;
                stream_copy(dst, src, copy_len);
                dst += s.RowPitch;
                src += copy_len;
                remaining -= copy_len;
            }
        }
        else {
            int8_t* dst = reinterpret_cast<int8_t*>(s.pData);
            auto src = sys_src;
            for (int i = 0; i < 1080; i++) {
                stream_copy(dst, src, row_bytes);
                dst += s.RowPitch;
                src += row_bytes;
            }
        }

        if (SUCCEEDED(hret)) {
            context->Unmap(tex.Get(), 0);
        }

        InterlockedAdd64(&ctx->gpu.counter, actual_copy / (1024 * 1024));

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
    std::cout << "usage: "<< argv[0] << " cpu_threads cpu_MB gpu_threads gpu_MB fps\n"
        << "  cpu_threads: number of CPU memcpy threads\n"
        << "  cpu_MB:      block size in MB for each CPU memcpy\n"
        << "  gpu_threads: number of GPU upload threads (D3D11 Map/Unmap)\n"
        << "  gpu_MB:      block size in MB for each GPU upload\n"
        << "  fps:         frame rate limit (omit for unlimited)\n"
        << "  example: " << argv[0] << " 1 64 2 8 30\n"
        << std::endl;

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
        << " [cpu] " << context.cpu.batch_size << " MB with " << context.cpu.threads << " threads and "
        << (context.cpu.fps > 0 ? std::to_string(1000 / context.cpu.fps) : "unlimited") << " fps \n"
        << " [gpu] " << context.gpu.batch_size << " MB with " << context.gpu.threads << " threads and "
        << (context.gpu.fps > 0 ? std::to_string(1000 / context.gpu.fps) : "unlimited") << " fps \n"
        << std::endl;

    for (auto i = 0; i < context.cpu.threads; i++) {
        std::thread td(memcpy_thread, &context);
        context.thread_objs.push_back(std::move(td));
    }

    for (auto i = 0; i < context.gpu.threads; i++) {
        std::thread td(mem_map_thread, &context);
        context.thread_objs.push_back(std::move(td));
    }

    int64_t prev_cpu_counter = 0;
    int64_t prev_gpu_counter = 0;

    while( context.machine.running ){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto cpu_mb = context.cpu.counter;
        auto cpu_gb = cpu_mb / 1024;
        auto delta_cpu_mb = cpu_mb - prev_cpu_counter;

        auto gpu_mb = context.gpu.counter;
        auto gpu_gb = gpu_mb / 1024;
        auto delta_gpu_mb = gpu_mb - prev_gpu_counter;

        prev_cpu_counter = cpu_mb;
        prev_gpu_counter = gpu_mb;

        context.machine.seconds++;
        auto sec = context.machine.seconds;
        std::cout << "excute in(" << sec << "): cpu total " << cpu_mb << " MB ("
            << cpu_gb << " GB), current " << delta_cpu_mb << " MB/s (" << delta_cpu_mb / 1024 << " GB/s)"
            << "\n        gpu total " << gpu_mb << " MB ("
            << gpu_gb << " GB), current " << delta_gpu_mb << " MB/s (" << delta_gpu_mb / 1024 << " GB/s)"
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
