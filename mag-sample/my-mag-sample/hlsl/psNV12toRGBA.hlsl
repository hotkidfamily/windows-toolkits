sampler YTex;
sampler UVTex;

struct PS_INPUT
{
    float2 y : TEXCOORD0;
    float2 uv : TEXCOORD1;
};


float4 main(PS_INPUT input) : COLOR0
{
    float y = tex2D(YTex, input.y).r;
    float2 uv = tex2D(UVTex, input.uv);
    float u = uv.r - 0.5f;
    float v = uv.g - 0.5f;

    float r = y + 1.14f * v;
    float g = y - 0.394f * u - 0.581f * v;
    float b = y + 2.03f * u;

    return float4(r, g, b, 1.0f);
}