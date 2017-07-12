float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}texture2D base : register(t0);
SamplerState samp : register(s0);


cbuffer LightBuffer
{
	float4 diffuseColor;
	float3 lightDirection;
	float  padding;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
	float3 worldPos : W_POS;
	float3 lightVal : COLOR;
};

float4 directional(PixelShaderInput input)
{
	float3x3 lightMovementZ = {
		cos(input.lightVal.x),-sin(input.lightVal.x),0.0f,
		sin(input.lightVal.x), cos(input.lightVal.x),0.0f,
		0.0f,0.0f,1.0f };
	float3 lightDirection = { 3.0f, -1.0f, 0.0f };
	lightDirection.x -= input.worldPos.x;
	lightDirection.y -= input.worldPos.y;
	lightDirection.z -= input.worldPos.z;
	lightDirection = mul(lightDirection, lightMovementZ);
	float lightRatio = saturate(dot(-normalize(lightDirection), input.normal));
	float3 color = { 0.5f, 0.1f, 0.1f };
	float4 temp = float4(color, 1.0f) * lightRatio;
	return temp;
}

float4 pLight(PixelShaderInput input) {
	float3 lightPos;
	float3x3 lightMovementY = { cos(input.lightVal.x),0.0f,sin(input.lightVal.x),
		0.0f,1.0f,0.0f,
		-sin(input.lightVal.x),0.0f, cos(input.lightVal.x) };
	float3 wPos = input.worldPos.xyz;

	wPos = mul(wPos, lightMovementY);

	lightPos.x = 0.0f - wPos.x;
	lightPos.y = 1.0f - wPos.y;
	lightPos.z = 3.0f - wPos.z;
	float mag = sqrt(lightPos.x*lightPos.x + lightPos.y*lightPos.y + lightPos.z*lightPos.z);
	float atenuation = 2.0f - saturate(mag / 10.0f);
	float3 lightDir = normalize(lightPos);
	float lightRatio = saturate(dot(normalize(lightDir), input.normal));
	float3 Color = { 0.2f, 0.75f, 0.2f };
	float4 temp = float4(Color, 1.0f)*lightRatio*atenuation;
	return temp;
}

float4 spot(PixelShaderInput input)
{
	float3 lightPos;
	float3x3 lightMovementY = {
		cos(input.lightVal.x),0.0f,sin(input.lightVal.x),
		0.0f,1.0f,0.0f,
		-sin(input.lightVal.x),0.0f, cos(input.lightVal.x)
	};
	float3 wPos = input.worldPos.xyz;

	wPos = mul(wPos, lightMovementY);

	lightPos.x = 0.0f - wPos.x;
	lightPos.y = 1.0f - wPos.y;
	lightPos.z = 0.0f - wPos.z;


	float3 Cone = { 0.0f, -1.0f, 0.0f };
	float3x3 lightMovementZ = {
		cos(input.lightVal.x),-sin(input.lightVal.x),0.0f,
		sin(input.lightVal.x), cos(input.lightVal.x),0.0f,
		0.0f,0.0f,1.0f
	};

	Cone = mul(Cone, lightMovementZ);

	float3 surfaceNormals = input.normal;

	float3 lightDir = normalize(lightPos);

	float3 surfaceRatio = saturate(dot(-normalize(lightDir), normalize(Cone)));

	float spotFactor = (surfaceRatio > 0.7f) ? 1 : 0;

	float lightRatio = saturate(dot(normalize(lightDir), input.normal));


	float3 Color = { 0.0f, 0.0f, 0.75f };
	float4 temp = float4(Color, 1.0f)*lightRatio*spotFactor;
	return temp;
}

float4 main(PixelShaderInput input) : sv_target
{
	float4 dl = directional(input);
	float4 pl = pLight(input);
	float4 sl = spot(input);
	float4 modelColor = base.Sample(samp, input.uv);	
	float4 sum = dl + pl; //+ sl
	return modelColor * sum;
	float4 temp = spot(input);

	//float4 temp = directional(input) + pLight(input);
	return temp;
}
