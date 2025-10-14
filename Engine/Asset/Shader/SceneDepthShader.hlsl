cbuffer DepthConstants : register(b0)
{
	row_major float4x4 InvViewProj;
	float3 CameraPosWS;
	float _pad0;
	float NearZ;
	float FarZ;
	float2 _pad1;
}

Texture2D DepthTexture : register(t0);
SamplerState SamplerPoint : register(s0);

struct VS_INPUT
{
	uint vertexID : SV_VertexID;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

// 선형 거리(카메라→표면)
float LinearEyeDistance(float2 uv)
{
	float depth01 = DepthTexture.SampleLevel(SamplerPoint, uv, 0).r;
	// D3Dxy(-1...1)z(0..1)->NDC(-1..1)

	float2 ndcXY = uv * 2.0f - 1.0f;   // -1..1
	float zClip = depth01*2-1;
	float4 clip  = float4(ndcXY, depth01, 1.0f); // z는 0..1 그대로

	float4 world = mul(clip, InvViewProj);
	world /= world.w;
	return distance(world.xyz, CameraPosWS);
}

PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output;

	// 풀스크린 삼각형
	float2 uv = float2((input.vertexID << 1) & 2, input.vertexID & 2);
	output.tex = uv;
	output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

	return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
	float d = LinearEyeDistance(input.tex);

	// 보기 편하게 범위 매핑(원하는 가시거리로 튜닝)
	float farVis = 150.0f;
	float v = saturate(d / farVis);

	return float4(v, v, v, v);
}
