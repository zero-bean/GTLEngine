#pragma once
#include "Vector.h"

//CBufferStruct 내부에 들어갈 Struct
struct FMaterialInPs
{
    FVector DiffuseColor; // Kd
    float OpticalDensity; // Ni

    FVector AmbientColor; // Ka
    float Transparency; // Tr Or d

    FVector SpecularColor; // Ks
    float SpecularExponent; // Ns

    FVector EmissiveColor; // Ke
    uint32 IlluminationModel; // illum. Default illumination model to Phong for non-Pbr materials

    FVector TransmissionFilter; // Tf
    float dummy; // 4 bytes padding

    FMaterialInPs() = default;
    FMaterialInPs(const FObjMaterialInfo& InMaterialInfo)
    {
        DiffuseColor = InMaterialInfo.DiffuseColor;
        AmbientColor = InMaterialInfo.AmbientColor;
        SpecularColor = InMaterialInfo.SpecularColor;
        SpecularExponent = InMaterialInfo.SpecularExponent;
        EmissiveColor = InMaterialInfo.EmissiveColor;
        Transparency = InMaterialInfo.Transparency;
        OpticalDensity = InMaterialInfo.OpticalDensity;
        IlluminationModel = InMaterialInfo.IlluminationModel;
        TransmissionFilter = InMaterialInfo.TransmissionFilter;
    }
};



#define CBUFFER_INFO(TYPENAME, SLOTNUM, SETVS, SETPS)\
constexpr uint32 TYPENAME##SlotNum = SLOTNUM;\
constexpr bool TYPENAME##SetVS = SETVS;\
constexpr bool TYPENAME##SetPS = SETPS;\

#define CBUFFER_TYPE_LIST(MACRO)\
MACRO(ModelBufferType)\
MACRO(ViewProjBufferType)\
MACRO(FViewProjectionInverse)\
MACRO(BillboardBufferType)\
MACRO(FPixelConstBufferType)\
MACRO(HighLightBufferType)\
MACRO(ColorBufferType)\
MACRO(UVScrollCB)\
MACRO(DecalMatrixCB)\
MACRO(ViewportBufferType)\
MACRO(DecalAlphaBufferType)\
MACRO(FHeightFogBufferType)\
MACRO(FPointLightBufferType)\
MACRO(FSpotLightBufferType)\
MACRO(CameraInfoBufferType)\
MACRO(FXAABufferType)\
MACRO(FGammaBufferType)\
MACRO(FDirectionalLightBufferType)\
MACRO(FSHAmbientLightBufferType)             \
MACRO(FMultiSHProbeBufferType)               \
MACRO(FBRDFInfoBufferType)                   \


CBUFFER_INFO(ModelBufferType, 0, true, false)
CBUFFER_INFO(ViewProjBufferType, 1, true, true)
CBUFFER_INFO(BillboardBufferType, 2, true, false)
CBUFFER_INFO(FViewProjectionInverse, 3, false, true)
CBUFFER_INFO(FPixelConstBufferType, 4, false, true)
CBUFFER_INFO(HighLightBufferType, 2, true, true)
CBUFFER_INFO(ColorBufferType, 3, false, true)
CBUFFER_INFO(UVScrollCB, 5, true, true)
CBUFFER_INFO(DecalMatrixCB, 7, false, true)
CBUFFER_INFO(ViewportBufferType, 6, false, true)

CBUFFER_INFO(DecalAlphaBufferType, 8, false, true)
CBUFFER_INFO(FHeightFogBufferType, 8, false, true)
CBUFFER_INFO(FPointLightBufferType, 9, true, true)
CBUFFER_INFO(FSpotLightBufferType, 13, true, true)
CBUFFER_INFO(FSHAmbientLightBufferType, 10, false, true)
CBUFFER_INFO(FMultiSHProbeBufferType, 12, false, true)
CBUFFER_INFO(CameraInfoBufferType, 0, false, true)
CBUFFER_INFO(FXAABufferType, 0, false, true)
CBUFFER_INFO(FGammaBufferType, 0, false, true)
CBUFFER_INFO(FDirectionalLightBufferType, 11, true, true) 
CBUFFER_INFO(FBRDFInfoBufferType, 6, true, true)

// VS : b0
struct ModelBufferType
{
    FMatrix Model;
    uint32 UUID = 0;
    FVector Padding;
    FMatrix NormalMatrix; // Inverse transpose of Model matrix for proper normal transformation
};

// VS : b1
struct ViewProjBufferType
{
    FMatrix View;
    FMatrix Proj;

    FVector CameraWorldPos; // 추가
    float  _pad_cam;       // 16바이트 정렬
};

// VS : b2
struct BillboardBufferType
{
    FVector pos;
    float padding;
    FMatrix InverseViewMat;
};

// PS : b3
struct FViewProjectionInverse
{
    FMatrix ViewProjectionInverse;
};

// PS : b4
struct FPixelConstBufferType
{
    FMaterialInPs Material;
    uint32 bHasMaterial; // 4 bytes (HLSL bool is 4 bytes)
    uint32 bHasTexture;  // 4 bytes (HLSL bool is 4 bytes)
    uint32 bHasNormal;
    float pad[2];           // 4 bytes padding for 16-byte alignment
};

struct HighLightBufferType
{
    uint32 Picked;
    FVector Color;
    uint32 X;
    uint32 Y;
    uint32 Z;
    uint32 Gizmo;
    uint32 enable;
};

// PS : b3
struct ColorBufferType
{
    FVector4 Color;
};

// PS : b5
struct UVScrollCB
{
    FVector2D Speed;
    float Time;
    float Pad;
};

//PS : b7
struct DecalMatrixCB
{
    FMatrix DecalWorldMatrix;
    FMatrix DecalWorldMatrixInv;
    FMatrix DecalProjectionMatrix;
    FVector DecalSize;
    float Padding;
};

// PS : b6
struct ViewportBufferType
{
    FVector4 ViewportRect; // x=StartX, y=StartY, z=SizeX, w=SizeY
};

#define MAX_POINT_LIGHTS 100

// PS : b6
struct alignas(16) FPointLightData
{
    FVector4 Position; // (x, y, z, radius)
    FVector4 Color;    // (r, g, b, falloff)
    float Radius;               // (안 써도 되지만 HLSL과 크기 맞춤용)
    float pad[3];               // 16바이트 맞춤 패딩
};
static_assert(sizeof(FPointLightData) == 48, "FPointLightData must be 48 bytes");

// 전체 버퍼 (cbuffer b9 대응)
struct alignas(16) FPointLightBufferType
{
    int PointLightCount;   // 4
    float _Padding[3];     // 12 → 총 16바이트
    FPointLightData PointLights[MAX_POINT_LIGHTS];
};
static_assert(sizeof(FPointLightBufferType) == 16 + 48 * MAX_POINT_LIGHTS, "PointLightBuffer size mismatch with HLSL");

// PS : b6
struct FSpotLightData
{
    FVector4 Position;   // xyz=위치, w=반경
    FVector4 Color;      // rgb=색상, a=falloff
    FVector4 Direction;

    float InnerConeAngle;       
    float OuterConeAngle;       
    float InAndOutSmooth;              // 16바이트 정렬 맞춤
    float _pad;
};

struct FSpotLightBufferType
{
    int SpotLightCount;                    // 활성 스포트 라이트 개수
    FVector _Padding;                       // 16바이트 정렬 (HLSL float3 pad)
    FSpotLightData SpotLights[MAX_POINT_LIGHTS]; // 배열 (최대 100)
};

// PS : b8
struct DecalAlphaBufferType
{
    float CurrentAlpha;
    FVector2D UVTiling;
    float pad;
};

// PS : b8
struct FHeightFogBufferType
{
    FLinearColor FogInscatteringColor;

    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;
    float FogMaxOpacityDistance;
    float FogMaxOpacity;
    float FogActorHeight;
    float Padding;
};

//PS : b10
// Spherical Harmonics L2 (2nd order) - 9 coefficients per RGB channel
// HLSL packs float3 arrays as float4 (16-byte aligned), so we use FVector4 to match
struct FSHAmbientLightBufferType
{
    FVector4 SHCoefficients[9];  // 9 RGB coefficients (w unused, but needed for alignment)
    float Intensity;             // Global intensity multiplier
    FVector Position;            // World position of ambient light probe
    float Radius;                // Influence radius (0 = global)
    FVector _Padding;            // 16-byte alignment
    float Falloff;               // Falloff exponent
};

// Single probe data for multi-probe system
struct FSHProbeData
{
    FVector4 Position;           // xyz=프로브 위치, w=BoxExtent.Z
    FVector4 SHCoefficients[9];  // 9개 SH 계수
    float Intensity;             // 강도
    float Falloff;               // 감쇠 지수
    FVector2D BoxExtent;         // xy=BoxExtent.X, BoxExtent.Y (Z는 Position.w에 저장)
};

#define MAX_SH_PROBES 8

// Multi-probe buffer (여러 프로브 지원)
struct FMultiSHProbeBufferType
{
    int ProbeCount;              // 활성 프로브 개수
    FVector _Padding;            // 16바이트 정렬
    FSHProbeData Probes[MAX_SH_PROBES];  // 최대 8개 프로브
};

//PS : b0
struct CameraInfoBufferType
{
    float NearClip;
    float FarClip;
    float Padding[2];
};

// PS : b0
struct FXAABufferType
{
    float SlideX = 1.0f;
    float SpanMax = 8.0f;
    float ReduceMin = 1.0f / 128.0f;
    float ReduceMul = 1.0f / 8.0f;
};

// PS : b0
struct FGammaBufferType
{
    float Gamma;
    FVector padding;
};

struct FDirectionalLightData
{
    FLinearColor Color;
    FVector Direction;
    int32 bEnableSpecular;
};

// PS : b11
struct FDirectionalLightBufferType
{
    int DirectionalLightCount;
    FVector Pad;
    FDirectionalLightData DirectionalLights[MAX_POINT_LIGHTS];
};
//---//




// PS : b6
struct FBRDFInfoBufferType
{
    float Roughness = 0.5f;
    float Metallic = 0.0f;
    FVector2D _Pad; // 16-byte alignment

    FBRDFInfoBufferType() = default;
    FBRDFInfoBufferType(float InRoughness, float InMetallic)
        : Roughness(InRoughness), Metallic(InMetallic), _Pad(0.0f, 0.0f) {}
};



