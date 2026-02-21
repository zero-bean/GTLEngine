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

float LinearEyeDistance(float2 localUV)
{
    float2 uv = GetFullUV(localUV);
    float depth01 = DepthTexture.SampleLevel(SamplerPoint, uv, 0).r;

    float2 ndcXY = ToNDCxy(uv);
	float  ndcZ  = depth01;
    float4 clip = float4(ndcXY, ndcZ, 1.0f);

    float4 world = mul(clip, InvViewProj);
    world /= world.w;

    return distance(world.xyz, CameraPosWSAndNear.xyz);
}

// 이 픽셀을 관통하는 Ray
float3 GetRayWS(float2 localUV)
{
	float2 uv = GetFullUV(localUV);

	const float zNearNDC = 0.0f; // D3D11 NDC
	const float zFarNDC  =  1.0f;

	float2 ndc = ToNDCxy(uv);

	float4 clipNear = float4(ndc, zNearNDC, 1.0f);
	float4 clipFar  = float4(ndc, zFarNDC,  1.0f);

	float4 worldNear = mul(clipNear, InvViewProj); worldNear /= worldNear.w;
	float4 worldFar  = mul(clipFar,  InvViewProj); worldFar  /= worldFar.w;

	return normalize(worldFar.xyz - worldNear.xyz);
}

float OpticalDepth_ExpHeight(float CameraZ, float RayZ, float WorldDepth,
							 float FogDensity, float FogHeightFalloff, float FogHeight,
							 float UnitToMeters)
{
	float k   = FogHeightFalloff;			// 1/unit
	float H   = FogHeight;					// unit
	float Lm  = WorldDepth * UnitToMeters;	// 스케일 일치 목적
	float z0  = CameraZ;
	float z1  = CameraZ + RayZ * WorldDepth;

	// RayZ가 거의 0이면 (수평 레이) 지수 부분이 거의 변하지 않으니(높지 않으니!)슬랩 근사
	const float EPS = 1e-6f;
	if (abs(RayZ) < EPS)
	{
		float zMid   = 0.5f * (z0 + z1);
		float sigmaM = FogDensity * exp(-k * (zMid - H));
		return sigmaM * Lm;
	}

	// 폐형식 적분:
	// tau_uu = FogDensity * exp(k*(H - z0)) * (1 - exp(-k*rdZ*Luu)) / (k*rdZ)
	// tau_m  = UnitToMeters * tau_uu  (단위 일치)
	float expo0 = exp(k * (H - z0));
	float expo1 = exp(-k * RayZ * WorldDepth);
	float tauUU = FogDensity * expo0 * (1.0f - expo1) / (k * RayZ);

	float tau = UnitToMeters * tauUU;

	// 과도 포화 방지 (수치 안정)
	return min(tau, 60.0f);
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
	// UV값을 World로 변환
	float WorldDepth = LinearEyeDistance(input.tex);

	// FogCutoffDistance 이상은 전부 FogCutoffDistance만큼으로 변환
	if (FogCutoffDistance > 0.0f)
	{
		WorldDepth = min(WorldDepth, FogCutoffDistance);
	}
	// StartDistance : 안개가 시작되는 첫 거리, 실제보다 거리를 더 가깝다고 계산
	if (StartDistance >0.0f)
	{
		WorldDepth = max(0.0f, WorldDepth - StartDistance);
	}
	if (WorldDepth <= 0.0f)
	{
		return float4(0,0,0,0);
	}

	float CameraZ = CameraPosWSAndNear.z;
	float3 WorldRay = GetRayWS(input.tex);

	const float UnitToMeters = 10.0f; // TODO: Engine에서 설정한 거리 단위 값. 추후 제작 후 CPU에서 넘겨줄 것

	// 빛 전달율 지수 Return
	float tau = OpticalDepth_ExpHeight(
		CameraZ,
		WorldRay.z,
		WorldDepth,
		FogDensity, FogHeightFalloff, FogHeight, UnitToMeters
	);

	// 빛 전달율 : 카메라~표면 사이 매질을 통과할 때, 원래 빛 몇%가 도달하는가? (0~1)
	// Transmittance : 투과율(Beer–Lambert) 적용
	float  Transmittance = exp(-tau);
	float alpha = saturate(min(1.0f - Transmittance, FogMaxOpacity));
	float3 fogColor = FogInscatteringColor.rgb;
	return float4(fogColor, alpha);
}
