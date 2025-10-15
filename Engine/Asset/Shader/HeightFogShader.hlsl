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

float2 LinearEyeDistance(float2 localUV)
{
    float2 uv = GetFullUV(localUV);
    float depth01 = DepthTexture.SampleLevel(SamplerPoint, uv, 0).r;

    float2 ndcXY = uv * 2.0f - 1.0f;
	ndcXY.y = -ndcXY.y; // UV 좌표계가 (0,0)=좌상단이지만, NDC는 (-1,1)=좌상단
	float  ndcZ  = depth01 * 2.0f - 1.0f;
    float4 clip = float4(ndcXY, ndcZ, 1.0f);

    float4 world = mul(clip, InvViewProj);
    world /= world.w;

    float2 result;
    result.x = distance(world.xyz, CameraPosWSAndNear.xyz);
    result.y = world.z;
    return result;
}

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    float2 uv = float2((input.vertexID << 1) & 2, input.vertexID & 2);
    output.tex = uv;
    output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float2 worldInfo = LinearEyeDistance(input.tex);
    float worldDepth = worldInfo.x;
    float worldZ = worldInfo.y;

	// 무한한 거리는 투명처리
	float depth01 = DepthTexture.SampleLevel(SamplerPoint, GetFullUV(input.tex), 0).r;
	if (depth01 >= 0.9999f)
	{
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	// FogCutoffDistance : 안개가 적용되는 최대 거리
	if (FogCutoffDistance > 0.0f && worldDepth > FogCutoffDistance)
	{
		return float4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// StartDistance : 안개가 시작되는 첫 거리, 실제보다 거리를 더 가깝다고 계산
	worldDepth = max(0.0f, worldDepth - StartDistance);

	// FogHeightFalloff : 안개 고도 감쇠 팩터, 클수록 급격하게 옅어진다.
	// FogDensity : 안개 밀도(0~1)
	// fogFactor : 클수록 안개 자욱, worldDepth : 클수록 안개 자욱, FogDensity : 클수록 안개 자욱
	float scatteringCoefficient = FogDensity * exp(-FogHeightFalloff * abs(worldZ)); // 0~1 * (1~e), 지수가 음수여야함
	float fogFactor = exp(-scatteringCoefficient * worldDepth);
	fogFactor = saturate(fogFactor);

	float3 fogColor = FogInscatteringColor.rgb;
	return float4(fogColor, 1-fogFactor);
	if (fogFactor >1)
		return float4(1.0f, 0.0f, 0.0f, 1.0f);
	else if (fogFactor >0.8)
		return float4(0.0f, 1.0f, 0.0f, 1.0f);
	else if (fogFactor >0.5)
		return float4(0.0f, 0.0f, 1.0f, 1.0f);
	else if (fogFactor >0.2)
		return float4(0.0f, 1.0f, 1.0f, 1.0f);
	return float4(1.0f, 1.0f, 0.0f, worldDepth);

	if (worldDepth >75)
		return float4(1.0f, 0.0f, 0.0f, 1.0f);
	else if (worldDepth >50)
		return float4(0.0f, 1.0f, 0.0f, 1.0f);
	else if (worldDepth >20)
		return float4(0.0f, 0.0f, 1.0f, 1.0f);
	else if (worldDepth >10)
		return float4(0.0f, 1.0f, 1.0f, 1.0f);
	return float4(1.0f, 1.0f, 0.0f, worldDepth);

	// // 과도한 포화 방지(수치 안정)
	// float transmittance = exp(-opticalDepth);
	// float alpha = saturate(min(1.0f - transmittance, FogMaxOpacity));
 //
}
