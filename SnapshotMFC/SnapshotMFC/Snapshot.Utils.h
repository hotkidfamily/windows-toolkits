#pragma once

#include <string>


namespace Snapshot
{
namespace Utils
{
int save_png_mem(uint8_t *src,
                 int width,
                 int height,
                 int channels,
                 int nw,
                 int nh,
                 uint8_t *&out,
                 int &out_len);
int free_mem(uint8_t *&out, int &out_len);

int save_bmp(uint8_t *src, int width, int height, int channels, int nw, int nh, std::wstring &path);
int save_png(uint8_t *src, int width, int height, int channels, int nw, int nh, std::wstring &path);
CRect RectPadding(CSize &src, CSize &exSz);
std::string WstringToUTF8(const std::wstring &wstr);

} // namespace Utils
} // namespace Snapshot
