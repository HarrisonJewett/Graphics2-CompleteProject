TextureCube base : register(t0);
SamplerState samp : register(s0);

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	return base.Sample(samp, normalize(input.uv));
}