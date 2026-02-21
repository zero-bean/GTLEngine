// LightIntensitySensor.hlsl - Compute Shader for Light Intensity Measurement
// =============================================================================
// 특정 월드 좌표들의 조명 세기(Luminance)를 측정하는 Compute Shader
// ULightSensorComponent에서 사용되며, Shadow Map을 포함한 정확한 조명 계산을 수행합니다.
// =============================================================================

#include "LightingFunctions.hlsli"

/*-----------------------------------------------------------------------------
    Constant Buffers (UberLit.hlsl과 동일한 레지스터 사용)
 -----------------------------------------------------------------------------*/

cbuffer Camera : register(b1)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    float3 ViewWorldLocation;
    float NearClip;
    float FarClip;
}

cbuffer GlobalLightConstant : register(b3)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
};

cbuffer ClusterSliceInfo : register(b4)
{
    uint ClusterSliceNumX;
    uint ClusterSliceNumY;
    uint ClusterSliceNumZ;
    uint LightMaxCountPerCluster;
    uint SpotLightIntersectOption;
    uint Orthographic;
    uint2 ClusterPadding;
};

cbuffer LightCountInfo : register(b5)
{
    uint PointLightCount;
    uint SpotLightCount;
    float2 CountPadding;
};

cbuffer CascadeShadowMapData : register(b6)
{
    row_major float4x4 CascadeView;
    row_major float4x4 CascadeProj[8];
    float4 SplitDistance[8];
    uint SplitNum;
    float BandingAreaFactor;
    float2 CascadePadding;
}

// Sensor 전용 입력 (배치 처리)
cbuffer SensorParams : register(b7)
{
    float4 SensorWorldPositions[32];  // FVector4와 정확히 일치 (w는 사용 안 함)
    uint SensorCount;
    float3 Padding;  // C++ 구조체와 정확히 일치
}

/*-----------------------------------------------------------------------------
    Structured Buffers (FLightPass가 바인딩한 것 재사용)
 -----------------------------------------------------------------------------*/

StructuredBuffer<int> PointLightIndices : register(t6);
StructuredBuffer<int> SpotLightIndices : register(t7);
StructuredBuffer<FPointLightInfo> PointLightInfos : register(t8);
StructuredBuffer<FSpotLightInfo> SpotLightInfos : register(t9);

/*-----------------------------------------------------------------------------
    Shadow Resources
 -----------------------------------------------------------------------------*/

Texture2D ShadowAtlas : register(t10);
Texture2D VarianceShadowAtlas : register(t11);
StructuredBuffer<FShadowAtlasTilePos> ShadowAtlasDirectionalLightTilePos : register(t12);
StructuredBuffer<FShadowAtlasTilePos> ShadowAtlasSpotLightTilePos : register(t13);
StructuredBuffer<FShadowAtlasPointLightTilePos> ShadowAtlasPointLightTilePos : register(t14);

/*-----------------------------------------------------------------------------
    Samplers
 -----------------------------------------------------------------------------*/

SamplerComparisonState ShadowSampler : register(s1);
SamplerState VarianceShadowSampler : register(s2);
SamplerState PointShadowSampler : register(s3);

/*-----------------------------------------------------------------------------
    Output
 -----------------------------------------------------------------------------*/

RWStructuredBuffer<float> OutputLuminance : register(u0);

/*-----------------------------------------------------------------------------
    Helper Functions
 -----------------------------------------------------------------------------*/

/**
 * @brief RGB 값을 Luminance로 변환 (ITU-R BT.709 표준)
 * @param rgb RGB 색상 값
 * @return 변환된 Luminance 값 (0.0 ~ 1.0+)
 */
float RGBToLuminance(float3 rgb)
{
    return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

/*-----------------------------------------------------------------------------
    Main Compute Shader
 -----------------------------------------------------------------------------*/

[numthreads(32, 1, 1)]
void main(uint3 ThreadID : SV_DispatchThreadID)
{
    // 범위 체크
    if (ThreadID.x >= SensorCount)
        return;

    // 센서 위치
    float3 WorldPos = SensorWorldPositions[ThreadID.x].xyz;

    // 기본 파라미터
    float3 WorldNormal = float3(0.0f, 0.0f, 1.0f);
    float Ns = 32.0f;

    float TotalLuminance = 0.0f;

    // 1. Ambient Light
    float4 AmbientColor = CalculateAmbientLight(Ambient);
    TotalLuminance += RGBToLuminance(AmbientColor.rgb);

    // 2. Directional Light
    FIllumination DirIllum = CalculateDirectionalLight(
        Directional,
        WorldNormal,
        WorldPos,
        ViewWorldLocation,
        Ns,
        ShadowAtlas,
        VarianceShadowAtlas,
        ShadowAtlasDirectionalLightTilePos,
        ShadowSampler,
        VarianceShadowSampler,
        View,
        CascadeView,
        CascadeProj,
        SplitDistance,
        SplitNum,
        BandingAreaFactor
    );
    TotalLuminance += RGBToLuminance(DirIllum.Diffuse.rgb);

    // 3. Point Lights (센서는 모든 방향의 빛을 받으므로 Normal을 빛 방향으로 설정)
    for (uint i = 0; i < PointLightCount; i++)
    {
        FPointLightInfo PointLight = PointLightInfos[i];

        // 빛 방향 계산
        float3 LightDir = PointLight.Position - WorldPos;
        float Distance = length(LightDir);

        // 범위 체크
        if (Distance < 1e-6 || Distance > PointLight.Range)
            continue;

        // Normal을 빛 방향으로 설정 (NdotL = 1이 되어 모든 방향 수용)
        float3 SensorNormal = SafeNormalize3(LightDir);

        FIllumination PointIllum = CalculatePointLight(
            PointLight,
            i,
            SensorNormal,
            WorldPos,
            ViewWorldLocation,
            Ns,
            ShadowAtlas,
            VarianceShadowAtlas,
            ShadowAtlasPointLightTilePos,
            PointShadowSampler
        );
        TotalLuminance += RGBToLuminance(PointIllum.Diffuse.rgb);
    }

    // 4. Spot Lights (Point Light와 동일한 패턴)
    for (uint j = 0; j < SpotLightCount; j++)
    {
        FSpotLightInfo SpotLight = SpotLightInfos[j];

        // 빛 방향 계산
        float3 LightDir = SpotLight.Position - WorldPos;
        float Distance = length(LightDir);

        // 범위 체크
        if (Distance < 1e-6 || Distance > SpotLight.Range)
            continue;

        // Normal을 빛 방향으로 설정 (NdotL = 1이 되어 모든 방향 수용)
        float3 SensorNormal = SafeNormalize3(LightDir);

        FIllumination SpotIllum = CalculateSpotLight(
            SpotLight,
            j,
            SensorNormal,
            WorldPos,
            ViewWorldLocation,
            Ns,
            ShadowAtlas,
            VarianceShadowAtlas,
            ShadowAtlasSpotLightTilePos,
            ShadowSampler,
            VarianceShadowSampler
        );
        TotalLuminance += RGBToLuminance(SpotIllum.Diffuse.rgb);
    }

    // 결과 저장
    OutputLuminance[ThreadID.x] = TotalLuminance;
}
