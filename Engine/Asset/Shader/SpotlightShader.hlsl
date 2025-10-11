cbuffer ModelConstantBuffer : register(b0)
{
	row_major float4x4 World; // 월드 행렬
	row_major float4x4 WorldInverseTranspose; // 월드 역전치 행렬
};

// C++의 FViewProjConstants와 일치
cbuffer ViewProjConstantBuffer : register(b1)
{
	row_major float4x4 View;
	row_major float4x4 Projection;
};

cbuffer ColorConstantBuffer : register(b2)
{
	float4 LightColor;
};

cbuffer LightConstantBuffer : register(b3)
{
	row_major float4x4 LightInverseWorld; // 데칼의 월드 역행렬
	row_major float4x4 LightWorld;
};

struct VS_INPUT
{
	float4 Position : POSITION;
	float3 Normal : NORMAL;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float4 WorldPos : TEXCOORD0;
	float3 WorldNormal : TEXCOORD1;
};


PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT) 0;

	// 1. 오브젝트의 로컬 좌표를 월드 좌표로 변환.
	output.WorldPos = mul(input.Position, World);
	// 2. 오브젝트의 로컬 노말을 월드 노말로 변환.
	output.WorldNormal = normalize(mul(input.Normal, (float3x3) WorldInverseTranspose));
	// 3. 오브젝트의 공간 정보를 최종 클립 좌표로 축소.
	output.Position = mul(mul(output.WorldPos, View), Projection);

	return output;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
	float3 lightForward = normalize(mul(float4(0, 0, -1, 0), LightWorld).xyz);
	// 1. Projector forward vector in world space (local -Z)
	if (dot(input.WorldNormal, lightForward) >= 0.0f)
	{
   		discard;
	}

	// Transform fragment position into light local space
	float3 localPos = mul(float4(input.WorldPos.xyz, 1.0f), LightInverseWorld).xyz;

	// Cone culling: base at local z = -0.5, tip at local z = +0.5
	const float coneHeight = 1.0f;      // length from -0.5 to +0.5 along local z
	const float baseRadius = 0.5f;      // radius at the base plane
	float depthFromTip = 0.5f - localPos.z; // distance from tip along -z

	if (depthFromTip < 0.0f || depthFromTip > coneHeight)
	{
   		discard;
	}

	float maxRadius = (depthFromTip / coneHeight) * baseRadius;
	float radius = length(localPos.xy);

	if (radius > maxRadius)
	{
   		discard;
	}

	return LightColor;
}
