#pragma once

#include <d3d11.h>
#include <cstdint>
#include <map>
#include <string>

#include "vframe.h"

enum MirrorMode
{
    MirrorMode_None,
    MirrorMode_Horizontal, // X
    MirrorMode_Vertical,   // Y
    MirrorMode_HorizontalPlusVertical,

    MirrorMode_X = MirrorMode_Horizontal,
    MirrorMode_Y = MirrorMode_Vertical,
    MirrorMode_XPlusY = MirrorMode_HorizontalPlusVertical
};

typedef struct FilterParams
{
    std::map<std::string, std::map<std::string, std::string>> args;
} FilterParams;

typedef struct FilterCtx
{
    ID3D11ComputeShader* CSs[4];
    uint32_t szCSs;
    ID3D11Buffer* CBs[4];
    uint32_t szCBs;
    ID3D11ShaderResourceView* SRVs[4];
    uint32_t szSRVs;
    ID3D11UnorderedAccessView* UAVs[12];
    uint32_t szUAVs;
    struct
    {
        uint32_t x;
        uint32_t y;
    }threads;

} FilterCtx;

class iFilter {
public:
    explicit iFilter(std::string n)
    {
        __name = n;
    };
    iFilter() = delete;
    virtual ~iFilter() {};

    virtual int init(FilterParams* params) = 0;
    virtual int process(GraphFrame* output, GraphFrame* input) = 0;

protected:
    std::string __name;
};