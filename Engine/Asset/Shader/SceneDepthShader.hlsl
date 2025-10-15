cbuffer FullscreenDepthConstants : register(b0)
{
	row_major float4x4 InvViewProj;
	float4 CameraPosWSAndNear; // xyz: camera position, w: near clip
	float4 ViewportRect;        // xy: normalized top-left, zw: normalized size
	float4 FarAndPadding;       // x: far clip, yzw: padding
}

cbuffer BendingMode : register(b1)
{
	float4 BendingMode; // x : 0 is Plane, 1 is Bending
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

// NDC z는 D3D11에선 0..1
float3 SolveWorldPosition(float2 localUV, out float depth01)
{
	float2 uv    = GetFullUV(localUV);
	depth01      = DepthTexture.SampleLevel(SamplerPoint, uv, 0).r;

	float2 ndcXY = ToNDCxy(uv);
	float  ndcZ  = depth01;
	float4 clip  = float4(ndcXY, ndcZ, 1.0f);

	float4 world = mul(clip, InvViewProj);
	world /= world.w;
	return world.xyz;
}

// 카메라 전방(WS) 구하기: 화면 중앙의 near/far를 역투영해 방향 벡터
float3 GetCameraForwardWS()
{
	float4 clipNear = float4(0, 0, 0, 1);  // D3D11: z=0
	float4 clipFar  = float4(0, 0, 1, 1);  // D3D11: z=1
	float4 pN = mul(clipNear, InvViewProj); pN /= pN.w;
	float4 pF = mul(clipFar , InvViewProj); pF /= pF.w;
	return normalize(pF.xyz - pN.xyz);
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
	float depth01;
	float3 WorldPosition = SolveWorldPosition(input.tex, depth01);

	if (BendingMode.x == 1.0f)
	{
		float3 cam = CameraPosWSAndNear.xyz;

		if (depth01 > 0.99999f)
			return float4(0.0, 0.0, 0.0, 0.0);

		const float SliceWidth = 15.0;
		float3 Forward = GetCameraForwardWS();
		float2 dir = normalize(Forward.xy + 1e-6);        // z 제거, XY 평면 방향만 1e-6은 0벡터 정규화 오류를 피하기 위해
		float planeDist = dot((WorldPosition-cam).xy, dir);     // 월드 XY 기준 띠

		float gray = frac(planeDist / SliceWidth);

		return float4(gray, gray, gray, 1.0);
	}

	float WorldDist = distance(WorldPosition.xyz, CameraPosWSAndNear.xyz);
	float farVis = 150.0f;
	float v = saturate(WorldDist/farVis);
	return float4(v, v, v, v);
}
