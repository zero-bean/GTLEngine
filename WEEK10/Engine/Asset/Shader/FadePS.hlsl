// FadePS.hlsl

#include "BlitVS.hlsl"

cbuffer FadeConstants : register(b1)
{
	float3 FadeColor;  // 예: (0, 0, 0) for black
	float  FadeAmount; // 0.0 (투명) ~ 1.0 (불투명)
};

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
	// CBuffer에서 읽은 값으로 바로 출력
	// FadeAmount가 픽셀의 Alpha 값이 됩니다.
	return float4(FadeColor, FadeAmount);
}
