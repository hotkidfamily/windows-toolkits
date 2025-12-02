Texture2D yTexture;
SamplerState texSampler; // Designed for nearest-neighbour

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
	float4 v = yTexture.Sample(texSampler, input.uv);
	
    return float4(v.b, v.g, v.r, 1.0f);
}