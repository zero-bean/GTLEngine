cbuffer FullscreenDepthConstants : register(b0)
{
	row_major float4x4 InvViewProj;
	float4 CameraPosWSAndNear; // xyz: camera position, w: near clip
	float4 ViewportRect;        // xy: normalized top-left, zw: normalized size
	float4 FarAndPadding;       // x: far clip, yzw: padding
}

cbuffer HeightFogConstants : register(b1)
{
	float FogDensity;
	float FogHeightFalloff;
	float StartDistance;
	float FogCutoffDistance;

	float FogMaxOpacity;
	float FogHeight;
	float _Pad0;
	float _Pad1;

	float4 FogInscatteringColor;
}

Texture2D DepthTexture : register(t0);
Texture2D SceneColorTex : register(t1);
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

// ViewRect와 UV Mapping
float2 GetFullUV(float2 localUV)
{
	return ViewportRect.xy + localUV * ViewportRect.zw;
}

// 선형 거리(카메라→표면)
float2 LinearEyeDistance(float2 localUV)
{
	float2 uv = GetFullUV(localUV);
	float depth01 = DepthTexture.SampleLevel(SamplerPoint, uv, 0).r;

	float2 ndcXY = uv * 2.0f - 1.0f; // -1 ~ 1
	float zClip = depth01 * 2.0f - 1.0f;
	float4 clip = float4(ndcXY, zClip, 1.0f);

	float4 world = mul(clip, InvViewProj);
	world /= world.w; // clip에서 w를 1로 줬으므로, 임의로 world에서 w를 나눠야한다.

	float2 outV;
	outV.x = distance(world.xyz, CameraPosWSAndNear.xyz);
	outV.y = world.y;
	return outV;
}

PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output;

	// 풀스크린 삼각형
	float2 uv = float2((input.vertexID << 1) & 2, input.vertexID & 2); // 00 02 20
	output.tex = uv;// * 0.5f;
	output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f); //-11 -13 -31

	return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
	float2 WorldDistanceAndY = LinearEyeDistance(input.tex);
	float WorldDepth = WorldDistanceAndY.x;
	float WorldY = WorldDistanceAndY.y;

	float depth01 = DepthTexture.SampleLevel(SamplerPoint, GetFullUV(input.tex), 0).r;
	if (depth01 >= 0.9999f) {
		// 하늘은 별도 처리: 안개 0 또는 멀리로 간주
		return SceneColorTex.Sample(SamplerPoint, GetFullUV(input.tex));
	}

	// 거리 컷인/컷오프
	float depth = max(0.0f, WorldDepth - StartDistance);
	if (FogCutoffDistance > 0.0f && WorldDepth > FogCutoffDistance)
	{
		float4 SceneColor = SceneColorTex.Sample(SamplerPoint, GetFullUV(input.tex));
		return SceneColor; // 또는 Alpha=0
	}

	float Sigma = FogDensity*exp(-FogHeightFalloff*(WorldY - FogHeight));
	Sigma = max(Sigma, 0.0f);

	// Beer-Lambert
	float T = exp(-Sigma*depth);
	float Alpha = saturate(min(1.0f - T, FogMaxOpacity));

	float4 SceneColor = SceneColorTex.Sample(SamplerPoint, GetFullUV(input.tex));
	float4 OutColor = lerp(SceneColor, FogInscatteringColor , Alpha);
	return SceneColor;
}
