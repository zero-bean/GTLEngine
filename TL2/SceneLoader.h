#pragma once

#include <fstream>
#include <sstream>

#include "nlohmann/json.hpp"
#include "Vector.h"
#include "UEContainer.h"
#include "LinearColor.h"
using namespace json;


// ========================================
// Version 1 (Legacy - 하위 호환용)
// ========================================
struct FPrimitiveData
{
    uint32 UUID = 0;
    FVector Location;
    FVector Rotation;
    FVector Scale;
    FString Type;
    FString ObjStaticMeshAsset;
};



struct FActorData
{
    uint32 UUID = 0;
    FString Type;  // "StaticMeshActor" 등
    FString Name;
    uint32 RootComponentUUID = 0;
};

struct FPerspectiveCameraData
{
    FVector Location;
	FVector Rotation;
	float FOV;
	float NearClip;
	float FarClip;
};

struct FSceneData
{
    uint32 Version = 2;
    uint32 NextUUID = 0;
    TArray<FActorData> Actors;
    TArray<FComponentData> Components;
    FPerspectiveCameraData Camera;
};

struct FLightComponentProperty
{
    float Intensity = 1.0f;
    float Temperature = 6500.0f;
    FLinearColor FinalColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    FLinearColor TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    FLinearColor TempColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    bool bEnableDebugLine = true;
};

struct FPointLightProperty
{    
    float Radius = 15.0f;             // 영향 반경
    float RadiusFallOff = 2.0f;       // 감쇠 정도 (클수록 급격히 사라짐)
    int Segments = 24;
};

struct FAmbientLightProperty
{    
    int32 CaptureResolution = 100;
    float UpdateInterval = 0.1f;
    float SmoothingFactor = 0.1f;
    float SHIntensity = 1.0f;
};

struct FDirectionalLightProperty
{    
    int32 bEnableSpecular = 1;
    FVector Direction;
};


struct FSpotLightProperty
{
    float InnnerConeAngle;
    float OuterConeAngle;
    float InAndOutSmooth;
    FVector4 Direction;
    int CircleSegments = 32;
};

struct FProjectileMovementProperty
{
    float InitialSpeed = 1000.f;
    float MaxSpeed = 3000.f;
    float GravityScale = 1.0f;
};


struct FRotationMovementProperty
{
    FVector RotationRate = FVector(0, 0, 0);
    FVector PivotTranslation = FVector(0, 0, 0);
    bool bRotationInLocalSpace = false;
};

struct FComponentData
{
    uint32 UUID = 0;
    uint32 OwnerActorUUID = 0;
    uint32 ParentComponentUUID = 0;  // 0이면 RootComponent (부모 없음)
    FString Type;  // "StaticMeshComponent", "AABoundingBoxComponent" 등

    // Transform
    FVector RelativeLocation;
    FVector RelativeRotation;
    FVector RelativeScale;

    // Type별 속성
    FString StaticMesh;  // StaticMeshComponent: Asset path
    TArray<FString> Materials;  // StaticMeshComponent: Materials
    TArray<FString> MaterialNormalMapOverrides;
    FString TexturePath;  // DecalComponent, BillboardComponent: Texture path
    FLightComponentProperty LightProperty;
    FPointLightProperty PointLightProperty; // PointLightComponent
    FSpotLightProperty SpotLightProperty;
    FAmbientLightProperty AmbientLightProperty; // AmbientLightComponent
    FDirectionalLightProperty DirectionalLightProperty; // DirectionalLightComponent
    // 신규
    FProjectileMovementProperty ProjectileMovementProperty;
    FRotationMovementProperty RotationMovementProperty;
};

class FSceneLoader
{
public:
    // Version 2 API
    static FSceneData LoadV2(const FString& FileName);
    static void SaveV2(const FSceneData& SceneData, const FString& SceneName);

    // Legacy Version 1 API (하위 호환)
    static TArray<FPrimitiveData> Load(const FString& FileName, FPerspectiveCameraData* OutCameraData);
    static void Save(TArray<FPrimitiveData> InPrimitiveData, const FPerspectiveCameraData* InCameraData, const FString& SceneName);

    static bool TryReadNextUUID(const FString& FilePath, uint32& OutNextUUID);

private:
    static TArray<FPrimitiveData> Parse(const JSON& Json);
    static FSceneData ParseV2(const JSON& Json);
};