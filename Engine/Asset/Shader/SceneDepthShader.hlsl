cbuffer FullscreenDepthConstants : register(b0)
{
	row_major float4x4 InvViewProj;
	float4 CameraPosWSAndNear; // xyz: camera position, w: near clip
	float4 ViewportRect;        // xy: normalized top-left, zw: normalized size
	float4 FarAndPadding;       // x: far clip, yzw: padding
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

float2 GetFullUV(float2 localUV)
{
	return ViewportRect.xy + localUV * ViewportRect.zw;
}

float2 ToNDCxy(float2 uv)
{
	return float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
}

// 선형 거리(카메라→표면)
float LinearEyeDistance(float2 localUV)
{
	float2 uv = GetFullUV(localUV);
	float depth01 = DepthTexture.SampleLevel(SamplerPoint, uv, 0).r;

	float2 ndcXY = ToNDCxy(uv);
	float  ndcZ  = depth01 * 2.0f - 1.0f;
	float4 clip = float4(ndcXY, ndcZ, 1.0f);

	float4 world = mul(clip, InvViewProj);
	world /= world.w;
	return distance(world.xyz, CameraPosWSAndNear.xyz);
}

PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output;

	// 풀스크린 삼각형
	float2 uv = float2((input.vertexID << 1) & 2, input.vertexID & 2);
	output.tex = uv;// * 0.5f;
	output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

	return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
	float d = LinearEyeDistance(input.tex);
	float Levels = 5.0;
	float Step = 1.0 / Levels;
	d %= Step;
	d*=Levels;
	return float4(d,d,d,d);
	//
	// float2 uv = GetFullUV(input.tex);
	// float depth01 = DepthTexture.SampleLevel(SamplerPoint, uv, 0).r;
	// float Levels = 5.0;
	// float Step = 1.0 / Levels;
	// // depth01 %= Step;
	// // depth01*=Levels;
	// float v  =depth01;
	// // return float4(v, v, v, v);

	// 보기 편하게 범위 매핑(원하는 가시거리로 튜닝)
	// float farVis = 150.0f;
	// v = saturate(d / farVis);
	// return float4(v, v, v, v);


	// // 2) 등고선(띠) 파라미터
	// const float BandMeters = 5.0f;        // 띠 간격: 5m
	// float t = d / BandMeters;             // 띠의 위상
	// float phase = frac(t);                // 0..1 주기
	//
	// // 화면 미분으로 경계 두께를 해상도-독립적으로
	// float w = fwidth(t) * 1.5;            // 두께 스케일은 취향대로
	// // 경계 0과 1 둘 다 잡기 (양쪽 에지)
	// float edge0 = 1.0 - smoothstep(0.0, w, phase);
	// float edge1 = smoothstep(1.0 - w, 1.0, phase);
	// float lineMask = saturate(edge0 + edge1); // 0=없음, 1=선
	//
	// // 3) 그레이와 등고선 합성 (선은 어둡게)
	// float3 baseCol = lerp(float3(0.05,0.05,0.05), float3(1,1,1), v);
	// float3 col = lerp(baseCol, float3(0,0,0), lineMask);
	//
	// return float4(col, 1);
}
