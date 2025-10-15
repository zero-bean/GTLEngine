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

float2 ToNDCxy(float2 uv)
{
	return float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
}

float2 LinearEyeDistance(float2 localUV)
{
    float2 uv = GetFullUV(localUV);
    float depth01 = DepthTexture.SampleLevel(SamplerPoint, uv, 0).r;

    float2 ndcXY = ToNDCxy(uv);
	float  ndcZ  = depth01 * 2.0f - 1.0f;
    float4 clip = float4(ndcXY, ndcZ, 1.0f);

    float4 world = mul(clip, InvViewProj);
    world /= world.w;

    float2 result;
    result.x = distance(world.xyz, CameraPosWSAndNear.xyz);
    result.y = world.z;
    return result;
}

void GetRayWS(float2 localUV, out float3 roWS, out float3 rdWS)
{
	float2 uv = GetFullUV(localUV);

	const float zNearNDC = -1.0f; // D3D11 NDC
	const float zFarNDC  =  1.0f;

	float2 ndc = ToNDCxy(uv);

	float4 clipNear = float4(ndc, zNearNDC, 1.0f);
	float4 clipFar  = float4(ndc, zFarNDC,  1.0f);

	float4 pNear = mul(clipNear, InvViewProj); pNear /= pNear.w;
	float4 pFar  = mul(clipFar,  InvViewProj); pFar  /= pFar.w;

	// 레이 시작점은 카메라 또는 근평면 중 택1. 카메라가 더 직관적임.
	roWS = CameraPosWSAndNear.xyz;
	rdWS = normalize(pFar.xyz - pNear.xyz);
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

	float3 roWS, rdWS;
	GetRayWS(input.tex, roWS, rdWS);

	float  depth01 = DepthTexture.SampleLevel(SamplerPoint, GetFullUV(input.tex), 0).r;
	bool miss = (depth01 >= 0.9999f);

	float nearZ = CameraPosWSAndNear.w;
	float farZ  = FarAndPadding.x;
	float L_virt_far = max(0.0f, farZ - nearZ);

	float Dist = miss ? L_virt_far : worldDepth;
	if (FogCutoffDistance > 0.0f)
	{
		Dist = min(Dist, FogCutoffDistance);
	}

	// StartDistance : 안개가 시작되는 첫 거리, 실제보다 거리를 더 가깝다고 계산
	Dist = max(0.0f, Dist - StartDistance);
	if (Dist <= 0.0f) return float4(0,0,0,0);

	float z0 = CameraPosWSAndNear.z;         // 카메라 z
	float3 ro, rd; GetRayWS(input.tex, ro, rd);
	float z1 = miss ? (ro.z + rd.z * Dist)      // 미스: 레이 끝 z
					: worldZ;                 // 히트: 표면 z

	float zClosest;
	// 구간이 z=0을 가로지르면(부호 다름) 최소 거리는 0
	if ((z0 >= 0 && z1 <= 0) || (z0 <= 0 && z1 >= 0))
		zClosest = 0.0f;
	else
		zClosest = min(abs(z0), abs(z1));    // 아니면 더 가까운 쪽

	// FogHeightFalloff : 안개 고도 감쇠 팩터, 클수록 급격하게 옅어진다.
	// FogDensity : 안개 밀도(0~1)
	float sigma  = FogDensity * exp(-FogHeightFalloff * (zClosest - FogHeight));
	const float UnitToMeters = 1.0f;
	float opticalDepth = min(sigma * (Dist * UnitToMeters), 60.0f);
	float fogFactor = exp(-opticalDepth);

	float alpha = saturate(min(1.0f - fogFactor, FogMaxOpacity));
	float3 fogColor = FogInscatteringColor.rgb;
	return float4(fogColor, alpha);

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
}
