cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;

	//this will break visual studios
	//float3 worldPos : W_POS;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
	float3 worldPos : W_POS;
	float3 lightVal : COLOR;
};

PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	pos = mul(pos, model);
	output.worldPos = pos;
	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;

	output.uv = input.uv;

	output.normal = mul(input.normal, (float3x3)model);

	//output.normal = normalize(output.normal);

	//position transformed by model matrix
	//position padded with a 1 in the w
	output.lightVal = (1.0f, 1.0f, 1.0f);

	return output;
}