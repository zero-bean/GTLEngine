// UberLit.hlsl - Uber Shader with Multiple Lighting Models
// Supports: Gouraud, Lambert, Phong lighting models

// =============================================================================
// <주의사항>
// normalize 대신 SafeNormalize 함수를 사용하세요.
// normalize에는 영벡터 입력시 NaN이 발생할 수 있습니다. (div by zero 가드가 없음)
// =============================================================================
#include "LightStructures.hlsli"
#include "LightingFunctions.hlsli"

#define NUM_POINT_LIGHT 8
#define NUM_SPOT_LIGHT 8
#define ADD_ILLUM(a, b) { (a).Ambient += (b).Ambient; (a).Diffuse += (b).Diffuse; (a).Specular += (b).Specular; }

// Constant Buffers
cbuffer Model : register(b0)
{
    row_major float4x4 World;
}

cbuffer Camera : register(b1)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    float3 ViewWorldLocation;
    float NearClip;
    float FarClip;
}

cbuffer MaterialConstants : register(b2)
{
    float4 Ka; // Ambient color
    float4 Kd; // Diffuse color
    float4 Ks; // Specular color
    float Ns;  // Specular exponent
    float Ni;  // Index of refraction
    float D;   // Dissolve factor
    uint MaterialFlags; // Which textures are available (bitfield)
    float Time;
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
    uint2 padding;
};

cbuffer LightCountInfo : register(b5)
{
    uint PointLightCount;
    uint SpotLightCount;
    float2 Padding;
};

cbuffer CascadeShadowMapData : register(b6)
{
    row_major float4x4 CascadeView;      // 64 bytes
    row_major float4x4 CascadeProj[8];   // 512 bytes
    float4 SplitDistance[8];    // 128 bytes (HLSL: each float is 16-byte aligned)
    uint SplitNum;             // 16 bytes (with padding)
    float BandingAreaFactor;
    // Total: 720 bytes
}

StructuredBuffer<int> PointLightIndices : register(t6);
StructuredBuffer<int> SpotLightIndices : register(t7);
StructuredBuffer<FPointLightInfo> PointLightInfos : register(t8);
StructuredBuffer<FSpotLightInfo> SpotLightInfos : register(t9);

uint GetPointLightCount(uint LightIndicesOffset)
{
    uint Count = 0;
    for (uint i = 0; i < LightMaxCountPerCluster; i++)
    {
        if (PointLightIndices[LightIndicesOffset + i] >= 0)
        {
            Count++;
        }
    }
    return Count;
}

uint GetPointLightIndex(uint LightIndicesIndex)
{
    return PointLightIndices[LightIndicesIndex];
}

FPointLightInfo GetPointLight(uint LightIdx)
{
    //uint LightInfoIdx = PointLightIndices[LightIdx];
    return PointLightInfos[LightIdx];
}

uint GetSpotLightCount(uint LightIndicesOffset)
{
    uint Count = 0;
    for (uint i = 0; i < LightMaxCountPerCluster; i++)
    {
        if (SpotLightIndices[LightIndicesOffset + i] >= 0)
        {
            Count++;
        }
    }
    return Count;
}

uint GetSpotLightIndex(uint LightIndicesIndex)
{
    return SpotLightIndices[LightIndicesIndex];
}

FSpotLightInfo GetSpotLight(uint LightIdx)
{
    // uint LightInfoIdx = SpotLightIndices[LightIdx];
    return SpotLightInfos[LightIdx];
}

// Textures
Texture2D DiffuseTexture : register(t0);
Texture2D AmbientTexture : register(t1);
Texture2D SpecularTexture : register(t2);
Texture2D NormalTexture : register(t3);
Texture2D AlphaTexture : register(t4);
Texture2D BumpTexture : register(t5);

// Shadow Maps
Texture2D ShadowAtlas : register(t10);
Texture2D VarianceShadowAtlas : register(t11);
StructuredBuffer<FShadowAtlasTilePos> ShadowAtlasDirectionalLightTilePos : register(t12);
StructuredBuffer<FShadowAtlasTilePos> ShadowAtlasSpotLightTilePos : register(t13);
StructuredBuffer<FShadowAtlasPointLightTilePos> ShadowAtlasPointLightTilePos : register(t14);

SamplerState SamplerWrap : register(s0);
SamplerComparisonState ShadowSampler : register(s1);
SamplerState VarianceShadowSampler : register(s2);
SamplerState PointShadowSampler : register(s3);

// Material flags
#define HAS_DIFFUSE_MAP  (1 << 0) // map_Kd
#define HAS_AMBIENT_MAP  (1 << 1) // map_Ka
#define HAS_SPECULAR_MAP (1 << 2) // map_Ks
#define HAS_NORMAL_MAP   (1 << 3) // map_normal
#define HAS_ALPHA_MAP    (1 << 4) // map_d
#define HAS_BUMP_MAP     (1 << 5) // map_Bump

// Vertex Shader Input/Output
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD0;
    float4 Tangent : TANGENT;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float2 Tex : TEXCOORD2;
    float4 WorldTangent : TEXCOORD3;
#if LIGHTING_MODEL_GOURAUD
    float4 AmbientLight : COLOR0;
    float4 DiffuseLight : COLOR1;
    float4 SpecularLight : COLOR2;
#endif
};

struct PS_OUTPUT
{
    float4 SceneColor : SV_Target0;
    float4 NormalData : SV_Target1;
};

float3 ComputeNormalMappedWorldNormal(float2 UV, float3 WorldNormal, float4 WorldTangent)
{
    float3 BaseNormal = SafeNormalize3(WorldNormal);

    // Tangent가 비정상(0 길이)이면 노말 맵 적용 포기하고 메시 노말 사용
    float TangentLen2 = dot(WorldTangent.xyz, WorldTangent.xyz);
    if (TangentLen2 <= 1e-8f)
    {
        return BaseNormal;
    }
    // 노말 맵 텍셀 샘플링 [0,1]
    float3 Encoded = NormalTexture.Sample(SamplerWrap, UV).xyz;
    // [0,1] -> [-1,1]로 매핑해서 탄젠트 공간 노말을 복원한다.
    float3 TangentSpaceNormal = SafeNormalize3(Encoded * 2.0f - 1.0f);

    // VS로 넘어온 월드 탄젠트를 정규화
    float3 T = WorldTangent.xyz / sqrt(TangentLen2);
    // TBN이 올바른 방향이 되도록 저장해둔 좌우손성으로 B 복원
    float Handedness = WorldTangent.w;
    float3 B = SafeNormalize3(cross(BaseNormal, T) * Handedness);

    float3x3 TBN = float3x3(T, B, BaseNormal);
    // 로컬 공간의 탄젠트를 월드 공간으로 보냄
    return SafeNormalize3(mul(TangentSpaceNormal, TBN));

}
// Vertex Shader
PS_INPUT Uber_VS(VS_INPUT Input)
{
    PS_INPUT Output;

    Output.WorldPosition = mul(float4(Input.Position, 1.0f), World).xyz;
    Output.Position = mul(mul(mul(float4(Input.Position, 1.0f), World), View), Projection);
    float3x3 World3x3 = (float3x3) World;
    Output.WorldNormal = SafeNormalize3(mul(Input.Normal, transpose(Inverse3x3(World3x3))));
    float3 WorldTangent = SafeNormalize3(mul(Input.Tangent.xyz, (float3x3) World));
    Output.WorldTangent = float4(WorldTangent, Input.Tangent.w);
    Output.Tex = Input.Tex;

#if LIGHTING_MODEL_GOURAUD
    // Calculate lighting in vertex shader (Gouraud)
    // Accumulate light only; material and textures are applied in pixel stage
    FIllumination Illumination = (FIllumination)0;

    // 1. Ambient Light
    Illumination.Ambient = CalculateAmbientLight(Ambient);

    // 2. Directional Light
    ADD_ILLUM(Illumination, CalculateDirectionalLight(Directional, Output.WorldNormal, Output.WorldPosition, ViewWorldLocation, Ns, ShadowAtlas, VarianceShadowAtlas, ShadowAtlasDirectionalLightTilePos, ShadowSampler, VarianceShadowSampler, View, CascadeView, CascadeProj, SplitDistance, SplitNum, BandingAreaFactor))

    // 3. Point Lights
    uint LightIndicesOffset = GetLightIndicesOffset(Output.WorldPosition, View, Projection, NearClip, FarClip, ClusterSliceNumX, ClusterSliceNumY, ClusterSliceNumZ, LightMaxCountPerCluster);
    uint PointLightCount = GetPointLightCount(LightIndicesOffset);
    [loop]
    for (uint i = 0; i < PointLightCount; i++)
    {
        uint LightIndex = GetPointLightIndex(LightIndicesOffset + i);
        FPointLightInfo PointLight = GetPointLight(LightIndex);
        ADD_ILLUM(Illumination, CalculatePointLight(PointLight, LightIndex, Output.WorldNormal, Output.WorldPosition, ViewWorldLocation, Ns, ShadowAtlas, VarianceShadowAtlas, ShadowAtlasPointLightTilePos, PointShadowSampler));
    }

    // 4.Spot Lights
    uint SpotLightCount = GetSpotLightCount(LightIndicesOffset);
    [loop]
    for (uint j = 0; j < SpotLightCount; j++)
    {
        uint LightIndex = GetSpotLightIndex(LightIndicesOffset + j);
        FSpotLightInfo SpotLight = GetSpotLight(LightIndex);
        ADD_ILLUM(Illumination, CalculateSpotLight(SpotLight, LightIndex, Output.WorldNormal, Output.WorldPosition, ViewWorldLocation, Ns, ShadowAtlas, VarianceShadowAtlas, ShadowAtlasSpotLightTilePos, ShadowSampler, VarianceShadowSampler));
    }

    // Assign to output
    Output.AmbientLight = Illumination.Ambient;
    Output.DiffuseLight = Illumination.Diffuse;
    Output.SpecularLight = Illumination.Specular;
#endif

    return Output;
}

// Pixel Shader
PS_OUTPUT Uber_PS(PS_INPUT Input)
{
    PS_OUTPUT Output;
    float4 finalPixel = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float2 UV = Input.Tex;
    float3 ShadedWorldNormal = SafeNormalize3(Input.WorldNormal);
    if (MaterialFlags & HAS_NORMAL_MAP)
    {
        ShadedWorldNormal = ComputeNormalMappedWorldNormal(UV, Input.WorldNormal, Input.WorldTangent);
        // else: Tangent가 유효하지 않으면 NormalBase 유지
    }
    // Sample textures
    float4 ambientColor = Ka;
    if (MaterialFlags & HAS_AMBIENT_MAP)
    {
        ambientColor *= AmbientTexture.Sample(SamplerWrap, UV);
    }
    else if (MaterialFlags & HAS_DIFFUSE_MAP)
    {
        // If no ambient map, but diffuse map exists, use diffuse map for ambient color
        ambientColor *= DiffuseTexture.Sample(SamplerWrap, UV);
    }

    float4 diffuseColor = Kd;
    if (MaterialFlags & HAS_DIFFUSE_MAP)
    {
        diffuseColor *= DiffuseTexture.Sample(SamplerWrap, UV);
    }

    float4 specularColor = Ks;
    if (MaterialFlags & HAS_SPECULAR_MAP)
    {
        specularColor *= SpecularTexture.Sample(SamplerWrap, UV);
    }


#if LIGHTING_MODEL_GOURAUD
    // Use pre-calculated vertex lighting; apply diffuse material/texture per-pixel
    finalPixel.rgb = Input.AmbientLight.rgb * ambientColor.rgb + Input.DiffuseLight.rgb * diffuseColor.rgb + Input.SpecularLight.rgb * specularColor.rgb;

#elif LIGHTING_MODEL_LAMBERT || LIGHTING_MODEL_BLINNPHONG
    // Calculate lighting in pixel shader
    FIllumination Illumination = (FIllumination)0;
    float3 N = ShadedWorldNormal;

    // 1. Ambient Light
    Illumination.Ambient = CalculateAmbientLight(Ambient);

    // 2. Directional Light
    ADD_ILLUM(Illumination, CalculateDirectionalLight(Directional, N, Input.WorldPosition, ViewWorldLocation, Ns, ShadowAtlas, VarianceShadowAtlas, ShadowAtlasDirectionalLightTilePos, ShadowSampler, VarianceShadowSampler, View, CascadeView, CascadeProj, SplitDistance, SplitNum, BandingAreaFactor));

    // 3. Point Lights
    uint LightIndicesOffset = GetLightIndicesOffset(Input.WorldPosition, View, Projection, NearClip, FarClip, ClusterSliceNumX, ClusterSliceNumY, ClusterSliceNumZ, LightMaxCountPerCluster);
    uint PointLightCount = GetPointLightCount(LightIndicesOffset);
    [loop]
    for (uint i = 0; i < PointLightCount ; i++)
    {
        uint LightIndex = GetPointLightIndex(LightIndicesOffset + i);
        FPointLightInfo PointLight = GetPointLight(LightIndex);
        ADD_ILLUM(Illumination, CalculatePointLight(PointLight, LightIndex, N, Input.WorldPosition, ViewWorldLocation, Ns, ShadowAtlas, VarianceShadowAtlas, ShadowAtlasPointLightTilePos, PointShadowSampler));
    }

    // 4. Spot Lights
    uint SpotLightCount = GetSpotLightCount(LightIndicesOffset);
    [loop]
    for (uint j = 0; j < SpotLightCount ; j++)
    {
        uint LightIndex = GetSpotLightIndex(LightIndicesOffset + j);
        FSpotLightInfo SpotLight = GetSpotLight(LightIndex);
        ADD_ILLUM(Illumination, CalculateSpotLight(SpotLight, LightIndex, N, Input.WorldPosition, ViewWorldLocation, Ns, ShadowAtlas, VarianceShadowAtlas, ShadowAtlasSpotLightTilePos, ShadowSampler, VarianceShadowSampler));
    }

    finalPixel.rgb = Illumination.Ambient.rgb * ambientColor.rgb + Illumination.Diffuse.rgb * diffuseColor.rgb + Illumination.Specular.rgb * specularColor.rgb;

#elif LIGHTING_MODEL_NORMAL
    float3 EncodedWorldNormal = ShadedWorldNormal * 0.5f + 0.5f;
    finalPixel.rgb = EncodedWorldNormal;

#else
    // Fallback: simple textured rendering (like current TexturePS.hlsl)
    finalPixel.rgb = diffuseColor.rgb + ambientColor.rgb;
#endif

    // Alpha handling
#if LIGHTING_MODEL_NORMAL
    finalPixel.a = 1.0f;
#else
    // 1. Diffuse Map 있으면 그 alpha 사용, 없으면 1.0
    float alpha = (MaterialFlags & HAS_DIFFUSE_MAP) ? diffuseColor.a : 1.0f;

    // 2. Alpha Map 따로 있으면 곱해줌.
    if (MaterialFlags & HAS_ALPHA_MAP)
    {
        alpha *= AlphaTexture.Sample(SamplerWrap, UV).r;
    }

    // 3. D 곱해서 최종 alpha 결정
    finalPixel.a = D * alpha;
#endif
    Output.SceneColor = finalPixel;

    // Encode normal for deferred rendering
    float3 encodedNormal = SafeNormalize3(ShadedWorldNormal) * 0.5f + 0.5f;
    Output.NormalData = float4(encodedNormal, 1.0f);

    return Output;
}
