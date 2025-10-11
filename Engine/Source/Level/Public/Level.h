#pragma once
#include "Core/Public/Object.h"
#include "Factory/Public/FactorySystem.h"
#include "Factory/Public/NewObject.h"

#include "Editor/Public/Camera.h"

class UWorld;

namespace json
{
	class JSON;
}

using JSON = json::JSON;

class AAxis;
class AGizmo;
class AGrid;
class AActor;
class UPrimitiveComponent;
class FFrustumCull;

/**
 * @brief Level Show Flag Enum
 */
enum class EEngineShowFlags : uint64
{
	SF_Primitives = 0x01,
	SF_BillboardText = 0x10,
	SF_Bounds = 0x20,
	SF_Decals = 0x40,
};

inline uint64 operator|(EEngineShowFlags lhs, EEngineShowFlags rhs)
{
	return static_cast<uint64>(lhs) | static_cast<uint64>(rhs);
}

inline uint64 operator&(uint64 lhs, EEngineShowFlags rhs)
{
	return lhs & static_cast<uint64>(rhs);
}

UCLASS()

class ULevel : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(ULevel, UObject)

public:
	ULevel();
	ULevel(const FName& InName);
	~ULevel() override
	{
		// 소멸자는 Cleanup 함수를 호출하여 모든 리소스를 정리하도록 합니다.
		Cleanup();
	}

	virtual void Init();
	virtual void Tick(float DeltaSeconds);
	virtual void Render();
	virtual void Cleanup();

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;

	const TArray<TObjectPtr<AActor>>& GetActors() const { return Actors; }

	const TArray<TObjectPtr<UPrimitiveComponent>>& GetLevelPrimitiveComponents() const
	{
		return LevelPrimitiveComponents;
	}

	TArray<TObjectPtr<UPrimitiveComponent>> GetVisiblePrimitiveComponents(UCamera* InCamera);

	void AddLevelPrimitiveComponentsInActor(AActor* Actor);
	void AddLevelPrimitiveComponent(TObjectPtr<UPrimitiveComponent> InPrimitiveComponent);
	void InitializeActorsInLevel();

	AActor* SpawnActorToLevel(UClass* InActorClass, const FName& InName = FName::GetNone());
	void RegisterDuplicatedActor(AActor* NewActor);

	bool DestroyActor(AActor* InActor);
	void MarkActorForDeletion(AActor* InActor);

	void SetSelectedActor(AActor* InActor);
	const TObjectPtr<AActor>& GetSelectedActor() const { return SelectedActor; }

	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

	// LOD Update System
	void UpdateLODForAllMeshes();
	void TickLODUpdate(float DeltaSeconds);

	// Graphics Quality Control
	void SetGraphicsQuality(int32 QualityLevel);

	// LOD Control Functions
	void SetGlobalLODEnabled(bool bEnabled);
	void SetMinLODLevel(int32 MinLevel);
	void SetLODDistance1(float Distance);
	void SetLODDistance2(float Distance);

	TObjectPtr<UWorld> GetOwningWorld() const { return OwningWorld; }
	void SetOwningWorld(const TObjectPtr<UWorld>& OwningWorld) { this->OwningWorld = OwningWorld; }

private:
	TArray<TObjectPtr<AActor>> Actors;
	TArray<TObjectPtr<UPrimitiveComponent>> LevelPrimitiveComponents; // 액터의 하위 컴포넌트는 액터에서 관리&해제됨

	FFrustumCull* Frustum = nullptr;

	// 자기를 가지고 있는 World
	TObjectPtr<UWorld> OwningWorld;

private:
	// 지연 삭제를 위한 리스트
	TArray<AActor*> ActorsToDelete;

	TObjectPtr<AActor> SelectedActor = nullptr;

	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
		static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
		static_cast<uint64>(EEngineShowFlags::SF_Bounds)|
		static_cast<uint64>(EEngineShowFlags::SF_Decals);

	// LOD Update System
	float LODUpdateFrameCounter = 0.f;
	static constexpr float LOD_UPDATE_INTERVAL = 0.2f; // 0.2초 마다 업데이트

	/**
	 * @brief Level에서 Actor를 실질적으로 제거하는 함수
	 * 이전 Tick에서 마킹된 Actor를 제거한다
	 */
	void ProcessPendingDeletions();
};
