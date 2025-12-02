
sampler YTex;
sampler UTex;
sampler VTex;

struct PS_INPUT
{
    float2 y : TEXCOORD0;
    float2 u : TEXCOORD1;
    float2 v : TEXCOORD2;
};

float4 main(PS_INPUT input) : COLOR0
{
    float y = tex2D(YTex, input.y).r;
    float u = tex2D(UTex, input.u.xy).r - 0.5f;
    float v = tex2D(VTex, input.v.xy).r - 0.5f;

    float r = y + 1.4f * v;
    float g = y - 0.343f * u - 0.711f * v;
    float b = y + 1.765f * u;

    return float4(r, g, b, 1.0f);
}