#pragma once

#undef min
#undef max

#include <cstdint>
#include <algorithm>
#include <memory>

#include <wrl/client.h>

enum PixelFormat
{
    PixelFormat_UNKNOW = 0x0 - 1,

    PixelFormat_GRAY = 0x0,
    PixelFormat_BAYERRGB_DECODE = 0x1,

    PixelFormat_RGBA = 0x10,
    PixelFormat_RGB24 = 0x20,

    PixelFormat_RGBA64LE = 0x30,
    PixelFormat_RGBA64BE = 0x31,

    PixelFormat_RGB48LE = 0x40,
    PixelFormat_RGB48BE = 0x41,

    PixelFormat_FLOAT16 = 0x50,
    PixelFormat_FLOAT32 = 0x51,
};

struct GraphFrame
{
    uint32_t w;
    uint32_t h;
    uint32_t bits;
    uint32_t stride;
    PixelFormat pix_fmt;
    struct
    {
        uint8_t* data;
    } cpumem;
    struct
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> textStage;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> text;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
    }gpumem;

    GraphFrame& operator=(GraphFrame& other)
    {
        w = other.w;
        h = other.h;
        bits = other.bits;
        stride = other.stride;
        pix_fmt = other.pix_fmt;
        cpumem.data = nullptr;
        gpumem.textStage = other.gpumem.textStage;
        gpumem.text = other.gpumem.text;
        gpumem.srv = other.gpumem.srv;
        gpumem.uav = other.gpumem.uav;
        gpumem.rtv = other.gpumem.rtv;
        return *this;
    }
};

static void copy_plane(uint8_t* dst, const uint32_t dst_pitch, uint8_t* src, const uint32_t src_pitch, int h)
{
    uint8_t* pSrc = (uint8_t*)src;
    uint8_t* pDest = (uint8_t*)dst;
    if (dst_pitch == src_pitch)
    {
        memmove(pDest, pSrc, dst_pitch * h);
    }
    else
    {
        auto cstride = std::min(src_pitch, dst_pitch);
        for (auto i = 0; i < h; i++)
        {
            memmove(pDest, pSrc, cstride);
            pDest += dst_pitch;
            pSrc += src_pitch;
        }
    }
}