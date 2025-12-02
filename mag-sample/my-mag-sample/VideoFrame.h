#pragma once

#include <stdint.h>

const int32_t kVideoFrameMaxPlane = 1;

enum class VideoFrameType
{
    kVideoFrameTypeRGBA = 0,
    kVideoFrameTypeBGRA,
    kVideoFrameTypeRGB24,
    kVideoFrameTypeI420,
    kVideoFrameTypeNV12,
}; 

class VideoFrame {
  public:

    static VideoFrame* MakeFrame(int32_t w, int32_t h, int32_t s, VideoFrameType type)
    {
        VideoFrame* frame = new VideoFrame(w, h, s, type);
        return frame;
    }

    ~VideoFrame()
    {
        if (_data) {
            _aligned_free(_data);
        }
        _data = nullptr;
    }

    VideoFrame()
        : _width(0)
        , _height(0)
        , _pixelType(VideoFrameType::kVideoFrameTypeRGBA)
        , _data(nullptr)
    {
        memset(_stride, 0, sizeof(_stride));
    }

    int32_t width() const 
    {
        return _width;
    }

    int32_t height() const
    {
        return _height;
    }

    int32_t stride() const
    {
        return _stride[0];    
    }

    int32_t bpp() const
    {
        return 4;
    }
    
    uint8_t* data() const
    {
        return _data;
    }

    VideoFrameType type() const
    {
        return _pixelType;
    }

  private:
    VideoFrame(int32_t w, int32_t h, int32_t s, VideoFrameType t)
    {
        _width = w;
        _height = h;
        _stride[0] = s;
        _pixelType = t;
        _data = (uint8_t *)_aligned_malloc(h * s, 32);
    }

    int32_t _width = 0;
    int32_t _height = 0;
    int32_t _stride[kVideoFrameMaxPlane];
    VideoFrameType _pixelType = VideoFrameType::kVideoFrameTypeRGBA;
    uint8_t *_data = nullptr;
};
