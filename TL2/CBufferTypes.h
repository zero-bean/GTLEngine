#pragma once
#include "Vector.h"

//CBufferStruct 안에 들어가는 Struct
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
    }
};




#define CBUFFER_INFO(TYPENAME, SLOTNUM, SETVS, SETPS)\
constexpr uint32 TYPENAME##SlotNum = SLOTNUM;\
constexpr bool TYPENAME##SetVS = SETVS;\
constexpr bool TYPENAME##SetPS = SETPS;\

#define CBUFFER_TYPE_LIST(MACRO)\
MACRO(ModelBufferType)					\
MACRO(ViewProjBufferType)					\
MACRO(FViewProjectionInverse)            \
MACRO(BillboardBufferType)					\
MACRO(FPixelConstBufferType)					\
MACRO(HighLightBufferType)					\
MACRO(ColorBufferType)					\
MACRO(UVScrollCB)					\
MACRO(DecalMatrixCB)					\
MACRO(ViewportBufferType)					\
MACRO(DecalAlphaBufferType)					\
MACRO(FHeightFogBufferType)                  \
MACRO(FPointLightBufferType)                  \
MACRO(CameraInfoBufferType)                  \
MACRO(FXAABufferType)                  \
MACRO(FGammaBufferType)                  \
MACRO(FDirectionalLightBufferType)          \

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
CBUFFER_INFO(FPointLightBufferType, 9, false, true)
CBUFFER_INFO(CameraInfoBufferType, 0, false, true)
CBUFFER_INFO(FXAABufferType, 0, false, true)
CBUFFER_INFO(FGammaBufferType, 0, false, true)
CBUFFER_INFO(FDirectionalLightBufferType, 11, false, true)


//Create 
//Release
//매크로를 통해 List에 추가하는 형태로 제작
//List를 통해 D3D11RHI
//Update
//Set vs,ps,hs,ds,gs

//VS : b0
struct ModelBufferType
{
    FMatrix Model;
    uint32 UUID = 0;
    FVector Padding;
    FMatrix NormalMatrix; // Inverse transpose of Model matrix for proper normal transformation
};

//VS : b1
struct ViewProjBufferType
{
    FMatrix View;
    FMatrix Proj;

    FVector CameraWorldPos; // ★ 추가
    float  _pad_cam;       // 16바이트 정렬용 패딩
};

//VS : b2
struct BillboardBufferType
{
    FVector pos;
    float padding;
    FMatrix InverseViewMat;
    /*FVector cameraRight;
    FVector cameraUp;*/
};

//PS : b3
struct FViewProjectionInverse
{
    FMatrix ViewProjectionInverse;
};


//PS : b4
struct FPixelConstBufferType
{
    FMaterialInPs Material;
    uint32 bHasMaterial; // 4 bytes (HLSL bool is 4 bytes)
    uint32 bHasTexture;  // 4 bytes (HLSL bool is 4 bytes)
    uint32 bHasNormalTexture; // 4 bytes for normal texture flag
    float pad;           // 4 bytes padding for 16-byte alignment
};

//VS,PS : b2
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

//PS : b3
struct ColorBufferType
{
    FVector4 Color;
};

//PS : b5
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

//PS : b6
struct ViewportBufferType
{
    FVector4 ViewportRect; // x=StartX, y=StartY, z=SizeX, w=SizeY
};
//PS : b6
struct FPointLightData
{
    FVector4 Position;   // xyz=위치, w=반경
    FVector4 Color;      // rgb=색상, a=Intensity
    float FallOff;       // 감쇠 정도
    FVector Padding;    // 16바이트 정렬 맞추기용
};
#define MAX_POINT_LIGHTS 100
// 전체 버퍼 (cbuffer b9 대응)
struct FPointLightBufferType
{
    int PointLightCount;                    // 현재 활성 조명 개수
    FVector _Padding;                      // 16바이트 정렬용 (HLSL의 float3 pad)
    FPointLightData PointLights[MAX_POINT_LIGHTS]; // 배열 (최대 100개)
};
//PS : b8
struct DecalAlphaBufferType
{
    float CurrentAlpha;
    FVector2D UVTiling;
    float pad;
};

//PS : b8
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

//PS : b0
struct CameraInfoBufferType
{
    float NearClip;
    float FarClip;
    float Padding[2];
};


//PS : b0
struct FXAABufferType
{
    float SlideX = 1.0f;
    float SpanMax = 8.0f;
    float ReduceMin = 1.0f / 128.0f;
    float ReduceMul = 1.0f / 8.0f;
};

//PS : b0
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





