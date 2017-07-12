texture2D base : register(t0);
SamplerState samp : register(s0);

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 temp = base.Sample(samp, input.uv);
	float4 color = temp;
	float gray = (color.r + color.g + color.b) / 10.0f;
	float4 final = (color.a, gray, gray, gray);

	if (false)
		return final;
	else
		return temp;
}