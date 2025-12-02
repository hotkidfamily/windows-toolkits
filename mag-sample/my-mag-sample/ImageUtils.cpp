#include "stdafx.h"
#include "ImageUtils.h"

namespace utils
{
void saveBmp(uint8_t *data, int width, int pictch, int height)
{
    auto leng = width * height * 4;

    std::unique_ptr<uint8_t> pBuf(new BYTE[leng]);
    UINT lBmpRowPitch = width * 4;
    auto sptr = static_cast<BYTE *>(data);
    auto dptr = pBuf.get() + leng - lBmpRowPitch;

    UINT lRowPitch = std::min<UINT>(lBmpRowPitch, pictch);

    for (size_t h = 0; h < height; ++h) {
        memcpy_s(dptr, lBmpRowPitch, sptr, lRowPitch);
        sptr += pictch;
        dptr -= lBmpRowPitch;
    }

    std::wstringstream ss;
    ss << "D:/temp/" << L"ScreenShot.bmp";

    auto f = std::make_unique<std::ofstream>(ss.str(), std::ios::out | std::ios::binary);

    if (f->is_open()) {
        BITMAPINFO bmpHead;

        ZeroMemory(&bmpHead, sizeof(BITMAPINFO));
        bmpHead.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmpHead.bmiHeader.biBitCount = 32;
        bmpHead.bmiHeader.biCompression = BI_RGB;
        bmpHead.bmiHeader.biWidth = width;
        bmpHead.bmiHeader.biHeight = height;
        bmpHead.bmiHeader.biPlanes = 1;
        bmpHead.bmiHeader.biSizeImage = width * height * 4;

        BITMAPFILEHEADER fileHead;

        fileHead.bfReserved1 = 0;
        fileHead.bfReserved2 = 0;
        fileHead.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpHead.bmiHeader.biSizeImage;
        fileHead.bfType = 'MB';
        fileHead.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        f->write((const char *)&fileHead, sizeof(BITMAPFILEHEADER));
        f->write((const char *)&bmpHead.bmiHeader, sizeof(BITMAPINFO));
        f->write((const char *)pBuf.get(), bmpHead.bmiHeader.biSizeImage);
    }
}

} // namespace utils