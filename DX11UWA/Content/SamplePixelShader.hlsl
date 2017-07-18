
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 WPos : W_POSITION;
	float3 uv : UV;
};


float4 main(PixelShaderInput input) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
	//return float4(input.uv, 1.0f);
}
