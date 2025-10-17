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

struct FPointLightProperty
{
    float Intensity = 5.0f;           // 밝기 (빛 세기)
    float Radius = 15.0f;             // 영향 반경
    float RadiusFallOff = 2.0f;       // 감쇠 정도 (클수록 급격히 사라짐)
    FLinearColor Color = FLinearColor(1.f, 0.0f, 0.0f, 1.f); // 오렌지빛
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
    FString TexturePath;  // DecalComponent, BillboardComponent: Texture path
    FPointLightProperty PointLightProperty; // PointLightComponent
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