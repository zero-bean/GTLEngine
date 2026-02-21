#pragma once
#include "Core/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"

namespace json { class JSON; }
using JSON = json::JSON;

UCLASS()
class UStaticMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

public:
	UStaticMeshComponent();
	~UStaticMeshComponent();

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;

public:
	UStaticMesh* GetStaticMesh() { return StaticMesh; }
	void SetStaticMesh(const FName& InObjPath);

	TObjectPtr<UClass> GetSpecificWidgetClass() const override;

	UMaterial* GetMaterial(int32 Index) const;
	void SetMaterial(int32 Index, UMaterial* InMaterial);

	// LOD System
	void SetLODLevel(int32 LODLevel);
	int32 GetCurrentLODLevel() const { return CurrentLODLevel; }
	void SetLODEnabled(bool bEnabled) { bLODEnabled = bEnabled; }
	bool IsLODEnabled() const { return bLODEnabled; }
	void UpdateLODBasedOnDistance(const FVector& CameraPosition);

	// LOD Distance Control
	void SetLODDistance1(float Distance) { LODDistanceSquared1 = Distance * Distance; }
	void SetLODDistance2(float Distance) { LODDistanceSquared2 = Distance * Distance; }
	float GetLODDistance1() const { return sqrtf(LODDistanceSquared1); }
	float GetLODDistance2() const { return sqrtf(LODDistanceSquared2); }
	float GetLODDistanceSquared1() const { return LODDistanceSquared1; }
	float GetLODDistanceSquared2() const { return LODDistanceSquared2; }

	// LOD Level Limit Control
	void SetMinLODLevel(int32 MinLevel) { MinLODLevel = MinLevel; }
	int32 GetMinLODLevel() const { return MinLODLevel; }

	// Forced LOD Level Control
	void SetForcedLODLevel(int32 ForcedLevel) { ForcedLODLevel = ForcedLevel; }
	int32 GetForcedLODLevel() const { return ForcedLODLevel; }
	bool IsForcedLODEnabled() const { return ForcedLODLevel >= 0; }

	void TickComponent(float DeltaSeconds) override;

	void EnableScroll() { bIsScrollEnabled = true; }
	void DisableScroll() { bIsScrollEnabled = false; }
	bool IsScrollEnabled() const { return bIsScrollEnabled; }

	void SetElapsedTime(float InElapsedTime) { ElapsedTime = InElapsedTime; }
	float GetElapsedTime() const { return ElapsedTime; }

private:
	TObjectPtr<UStaticMesh> StaticMesh;

	// MaterialList
	TArray<UMaterial*> OverrideMaterials;

	// LOD System
	int32 CurrentLODLevel = 0;
	bool bLODEnabled = true;
	float LODDistanceSquared1 = 400.0f;  // LOD 1 전환 거리 제곱 (20^2)
	float LODDistanceSquared2 = 1600.0f; // LOD 2 전환 거리 제곱 (40^2)
	int32 MinLODLevel = 0;  // 최소 허용 LOD 레벨 (0=모든LOD, 1=LOD1,2만, 2=LOD2만)
	int32 ForcedLODLevel = -1;  // 강제 LOD 레벨 (-1=자동, 0~2=강제 LOD)
	FName OriginalMeshPath;  // 원본 메시 경로 저장

	// Scroll
	bool bIsScrollEnabled;
	float ElapsedTime;
};
