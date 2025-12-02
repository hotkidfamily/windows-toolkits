struct VSInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput main(VSInput input)
{
    PSInput ot;
    ot.pos = float4(input.pos, 1.0f);
    ot.uv = input.uv.xy;
    return ot;
};