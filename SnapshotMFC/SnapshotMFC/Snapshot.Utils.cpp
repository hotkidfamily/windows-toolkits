#include "pch.h"
#include "Snapshot.Utils.h"

#include <string>

#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize2.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"


namespace Snapshot
{
namespace Utils
{
int save_png_mem(uint8_t *src, int width, int height, int channels, int nw, int nh, uint8_t* &out, int &out_len)
{
    if (width != nw && height != nh)
    {
        uint8_t *resized = (uint8_t *)malloc(nw * nh * channels);

        stbir_resize_uint8_srgb(src, width, height, width * channels, resized, nw, nh,
                                nw * channels, STBIR_BGRA);
        for (int i = 0; i < nw * nh; i++)
        {
            BYTE tmp = resized[i * 4];
            resized[i * 4] = resized[i * 4 + 2]; // 交换R和B
            resized[i * 4 + 2] = tmp;
        }
        out = stbi_write_png_to_mem(resized, nw*4, nw, nh, channels, &out_len);
        free(resized);
    }
    else
    {
        for (int i = 0; i < nw * nh; i++)
        {
            BYTE tmp = src[i * 4];
            src[i * 4] = src[i * 4 + 2]; // 交换R和B
            src[i * 4 + 2] = tmp;
            src[i * 4 + 3] = 0xff;
        }
        out = stbi_write_png_to_mem(src,nw*4, nw, nh, channels, &out_len);
    }

    return 0;
}

int free_mem(uint8_t *&out, int &out_len)
{
    if (out)
        STBIW_FREE(out);
    out = nullptr;
    out_len = 0;
    return 0;
}

int save_png(uint8_t *src, int width, int height, int channels, int nw, int nh, std::wstring &path)
{
    uint8_t *resized = (uint8_t *)malloc(nw * nh * channels);

    stbir_resize_uint8_srgb(src, width, height, width * channels, resized, nw, nh, nw * channels,
                            STBIR_BGRA);
    for (int i = 0; i < nw * nh; i++)
    {
        BYTE tmp = resized[i * 4];
        resized[i * 4] = resized[i * 4 + 2]; // 交换R和B
        resized[i * 4 + 2] = tmp;
        resized[i * 4 + 3] = 0xff;
    }
    std::string a = WstringToUTF8(path);
    stbi_write_png(a.c_str(), nw, nh, channels, resized, nw * 4);
    free(resized);

    return 0;
}

int save_bmp(uint8_t *src, int width, int height, int channels, int nw, int nh, std::wstring &path)
{
    if (width != nw && height != nh)
    {
        uint8_t *resized = (uint8_t *)malloc(nw * nh * channels);

        stbir_resize_uint8_srgb(src, width, height, width * channels, resized, nw, nh,
                                nw * channels, STBIR_BGRA);
        for (int i = 0; i < nw * nh; i++)
        {
            BYTE tmp = resized[i * 4];
            resized[i * 4] = resized[i * 4 + 2]; // 交换R和B
            resized[i * 4 + 2] = tmp;
        }
        std::string a = WstringToUTF8(path);
        stbi_write_bmp(a.c_str(), nw, nh, channels, resized);
        free(resized);
    }
    else
    {
        for (int i = 0; i < nw * nh; i++)
        {
            BYTE tmp = src[i * 4];
            src[i * 4] = src[i * 4 + 2]; // 交换R和B
            src[i * 4 + 2] = tmp;
        }
        std::string a = WstringToUTF8(path);
        stbi_write_bmp(a.c_str(), nw, nh, channels, src);
    }

    return 0;
}

std::string WstringToUTF8(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();

    int size_needed
        = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, nullptr,
                        nullptr);
    return strTo;
}

CRect RectPadding(CSize &src, CSize &exSz)
{
    CSize exportSz{ exSz.cx, exSz.cy };
    CRect desRect{};
    {
        float vp_width = 0;
        float vp_height = 0;
        float vp_top = 0;
        float vp_left = 0;

        float frameRatio = src.cx * 1.0f / src.cy;
        float wndRatio = exportSz.cx * 1.0f / exportSz.cy;

        if (wndRatio > frameRatio)
        {
            vp_width = exportSz.cy * frameRatio;
            vp_height = exportSz.cy;
            vp_left = (exportSz.cx - vp_width) / 2;
            vp_top = 0;
        }
        else
        {
            vp_width = exportSz.cx;
            vp_height = exportSz.cx / frameRatio;
            vp_left = 0;
            vp_top = (exportSz.cy - vp_height) / 2;
        }
        desRect
            = { (int)vp_left, (int)vp_top, (int)(vp_left + vp_width), (int)(vp_top + vp_height) };
    }

    return desRect;
}

} // namespace Utils
} // namespace Snapshot
