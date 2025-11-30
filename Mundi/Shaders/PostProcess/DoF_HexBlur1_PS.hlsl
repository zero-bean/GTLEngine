#include "../Common/PostProcessCommon.hlsl"

Texture2D<float4> g_InputTexture : register(t0);    // 이전 패스의 결과 또는 원본 씬 컬러
Texture2D<float4> g_CoCTexture : register(t1);      // CoC Map (R=Far, G=Near, A=Depth)
SamplerState g_LinearSampler : register(s0);

// 1차 패스: 0도 방향 블러 (수평)
#define BLUR_DIRECTION float2(1.0, 0.0) 

float4 mainPS(float4 position : SV_Position, float2 texcoord : TEXCOORD0) : SV_Target
{
    // CoC 맵에서 현재 픽셀의 CoC 값과 깊이 값을 읽어옴
    float4 cocSample = g_CoCTexture.Sample(g_LinearSampler, texcoord);
    float farCoC = cocSample.r;
    float nearCoC = cocSample.g;
    float centerDepth = cocSample.a;  // 현재 픽셀의 깊이

    // Near, Far 중 더 큰 CoC 값을 블러 반경으로 사용
    float cocRadius = max(nearCoC, farCoC);

    // 초점 영역(CoC가 거의 0)이면 원본 이미지 그대로 사용
    if (cocRadius < 0.01)
    {
        float4 originalColor = g_InputTexture.Sample(g_LinearSampler, texcoord);
        originalColor.a = cocSample.a;
        return originalColor;
    }

    float4 finalColor = float4(0.0, 0.0, 0.0, 0.0);
    float totalWeight = 0.0;

    int sampleCount = FDepthOfFieldBuffer.BlurSampleCount;
    if (sampleCount == 0) sampleCount = 1;

    float blurScale = cocRadius * FDepthOfFieldBuffer.MaxCoc;

    // 깊이 가중치 임계값 (조정 가능)
    // 0.001 = 매우 엄격 (블러 거의 없음, 윤곽선 완벽 보호)
    // 0.01 = 엄격 (윤곽선 보호, 적당한 블러)
    // 0.05 = 보통 (부드러운 전환)
    // 0.1 = 관대 (자연스러운 블러, 약한 보호)
    float depthThreshold = 0.01;

    [loop]
    for (int i = -sampleCount; i <= sampleCount; ++i)
    {
        float2 offset = BLUR_DIRECTION * (i / (float)sampleCount) * blurScale * ScreenSize.zw;
        float2 sampleUV = texcoord + offset;

        // 샘플 위치의 깊이 읽기
        float4 sampleCoc = g_CoCTexture.Sample(g_LinearSampler, sampleUV);
        float sampleDepth = sampleCoc.a;

        // 거리 기반 가중치 (기존)
        float distanceWeight = 1.0 - abs(i) / (float)sampleCount;

        // 깊이 차이 기반 가중치 (새로 추가)
        float depthDiff = abs(centerDepth - sampleDepth);
        float depthWeight = exp(-depthDiff / depthThreshold);

        // 최종 가중치 = 거리 × 깊이
        float finalWeight = distanceWeight * depthWeight;

        // 색상 샘플링
        float4 sampleColor = g_InputTexture.Sample(g_LinearSampler, sampleUV);

        finalColor += sampleColor * finalWeight;
        totalWeight += finalWeight;
    }

    if (totalWeight > 0.0)
    {
        finalColor /= totalWeight;
    }
    else
    {
        finalColor = g_InputTexture.Sample(g_LinearSampler, texcoord);
    }

    finalColor.a = cocSample.a;

    return finalColor;
}