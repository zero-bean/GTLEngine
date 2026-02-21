#pragma once
#include "Core/Public/Object.h"
#include "Global/Enum.h"

class UWorld;
class AActor;
class UPrimitiveComponent;
class UPointLightComponent;
class ULightComponent;
class FOctree;
class UCurveLibrary;

// Custom hash function for pair of WeakObjectPtr (used in overlap tracking)
struct PairHash
{
	template <typename T>
	std::size_t operator()(const std::pair<TWeakObjectPtr<T>, TWeakObjectPtr<T>>& Pair) const
	{
		// WeakObjectPtr.Get()으로 raw pointer를 가져와서 해시
		std::size_t H1 = std::hash<T*>{}(Pair.first.Get());
		std::size_t H2 = std::hash<T*>{}(Pair.second.Get());
		return H1 ^ (H2 << 1); // Simple hash combination
	}
};

UCLASS()
class ULevel :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(ULevel, UObject)
public:
	ULevel();
	~ULevel() override;

	virtual void Init();

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	const TArray<AActor*>& GetLevelActors() const { return LevelActors; }
	const TArray<AActor*>& GetTemplateActors() const { return TemplateActors; }

	void AddActorToLevel(AActor* InActor);
	void RegisterTemplateActor(AActor* InActor);
	void UnregisterTemplateActor(AActor* InActor);

	// Template Actor 검색 함수
	AActor* FindTemplateActorByName(const FName& InName) const;
	TArray<AActor*> FindTemplateActorsOfClass(UClass* InClass) const;

	// Regular Actor 검색 함수 (Template Actor 제외)
	AActor* FindActorByName(const FName& InName) const;
	TArray<AActor*> FindActorsOfClass(UClass* InClass) const;
	TArray<AActor*> FindActorsOfClassByName(const FString& ClassName) const;

	void AddLevelComponent(AActor* Actor);

	void RegisterComponent(UActorComponent* InComponent);
	void UnregisterComponent(UActorComponent* InComponent);
	bool DestroyActor(AActor* InActor);

	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

	void UpdatePrimitiveInOctree(UPrimitiveComponent* InComponent);

	FOctree* GetStaticOctree() { return StaticOctree; }

	/**
	 * @brief Octree 범위 밖에 있는 동적 프리미티브 목록 반환
	 * @return 동적 프리미티브 배열 (값 복사)
	 * @note 참조 무효화 방지를 위해 값으로 반환 (각 호출자가 독립적인 복사본 획득)
	 * @todo 성능 최적화: DirtyFlag와 캐시된 배열 도입
	 *       - 멤버 변수: TArray<UPrimitiveComponent*> CachedDynamicPrimitives
	 *       - 플래그: bool bIsDynamicPrimitivesCacheDirty
	 *       - OnPrimitiveUpdated/OnPrimitiveUnregistered에서 DirtyFlag 설정
	 *       - GetDynamicPrimitives()에서 캐시가 유효하면 재사용
	 *       - 예상 성능 개선: 매 프레임 복사 비용 제거 (특히 다중 호출 시)
	 */
	TArray<UPrimitiveComponent*> GetDynamicPrimitives() const
	{
		TArray<UPrimitiveComponent*> Result;
		Result.Reserve(DynamicPrimitiveMap.Num());

		for (auto [Component, TimePoint] : DynamicPrimitiveMap)
		{
			Result.Add(Component);
		}
		return Result;
	}

	friend class UWorld;

	// Owning World
	UWorld* GetOwningWorld() const { return OwningWorld; }

	// Curve Library
	UCurveLibrary* GetCurveLibrary() const { return CurveLibrary; }

public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

private:
	AActor* SpawnActorToLevel(UClass* InActorClass, JSON* ActorJsonData = nullptr);

	UWorld* OwningWorld = nullptr; // 이 레벨을 소유한 World
	UCurveLibrary* CurveLibrary = nullptr; // Curve repository
	TArray<AActor*> LevelActors;	// 레벨이 보유하고 있는 모든 Actor를 배열로 저장합니다.
	TArray<AActor*> TemplateActors;	// bIsTemplate이 true인 Actor들의 캐시 (빠른 조회용)

	// 지연 삭제를 위한 리스트
	TArray<AActor*> ActorsToDelete;

	uint64 ShowFlags =
		static_cast<uint64>(EEngineShowFlags::SF_Billboard) |
		static_cast<uint64>(EEngineShowFlags::SF_StaticMesh) |
		static_cast<uint64>(EEngineShowFlags::SF_Text) |
		static_cast<uint64>(EEngineShowFlags::SF_Decal) |
		static_cast<uint64>(EEngineShowFlags::SF_Fog) |
		static_cast<uint64>(EEngineShowFlags::SF_Collision) |
		static_cast<uint64>(EEngineShowFlags::SF_FXAA)  |
		static_cast<uint64>(EEngineShowFlags::SF_SkeletalMesh);

	/*-----------------------------------------------------------------------------
		Octree Management
	-----------------------------------------------------------------------------*/
public:
	void UpdateOctree();
	void UpdateOctreeImmediate();

private:

	void OnPrimitiveUpdated(UPrimitiveComponent* InComponent);

	void OnPrimitiveUnregistered(UPrimitiveComponent* InComponent);

	/** @brief 한 프레임에 Octree에 삽입할 오브젝트의 최대 크기를 결정해서 부하를 여러 프레임에 분산함. */
	static constexpr uint32 MAX_OBJECTS_TO_INSERT_PER_FRAME = 256;

	/** @brief 가장 오래전에 움직인 UPrimitiveComponent를 Octree에 삽입하기 위해 필요한 구조체. */
	struct FDynamicPrimitiveData
	{
		UPrimitiveComponent* Primitive;
		float LastMoveTimePoint;

		bool operator>(const FDynamicPrimitiveData& Other) const
		{
			return LastMoveTimePoint > Other.LastMoveTimePoint;
		}
	};

	using FDynamicPrimitiveQueue = TQueue<FDynamicPrimitiveData>;

	FOctree* StaticOctree = nullptr;

	/** @brief 가장 오래전에 움직인 UPrimitiveComponent부터 순서대로 Octree에 삽입할 수 있도록 보관 */
	FDynamicPrimitiveQueue DynamicPrimitiveQueue;

	/** @brief 각 UPrimitiveComponent가 움직인 가장 마지막 시간을 기록 */
	TMap<UPrimitiveComponent*, float> DynamicPrimitiveMap;

	/*-----------------------------------------------------------------------------
		Lighting Management
	-----------------------------------------------------------------------------*/
public:
	const TArray<ULightComponent*>& GetLightComponents() const { return LightComponents; }

private:
	TArray<ULightComponent*> LightComponents;

	/*-----------------------------------------------------------------------------
		Centralized Overlap Management (Unreal-style)
	-----------------------------------------------------------------------------*/
public:
	/**
	 * @brief 중앙집중식 overlap 업데이트 (Unreal Engine 방식)
	 * @note World::Tick()에서 호출되며, 모든 primitive component의 overlap을 한번에 체크
	 *       - Component tick 여부와 무관하게 동작
	 *       - 양방향 상태 + 이벤트 동기화
	 *       - 중복 overlap 테스트 방지 (각 pair는 한 번만 체크)
	 */
	void UpdateAllOverlaps();

	/**
	 * @brief 컴포넌트가 특정 위치로 이동했을 때 첫 번째로 충돌하는 컴포넌트 반환 (실제 이동 없이 테스트만)
	 * @param Component 테스트할 컴포넌트
	 * @param TargetLocation 이동할 목표 위치 (world space)
	 * @param OutHit 충돌 정보 (충돌한 컴포넌트, 액터 등)
	 * @return 충돌이 감지되면 true
	 * @note 이동 전 미리 호출하여 충돌 여부 확인 (예: 벽 통과 방지, 가장 가까운 충돌만 필요한 경우)
	 */
	bool SweepComponentSingle(
		UPrimitiveComponent* Component,
		const FVector& TargetLocation,
		struct FHitResult& OutHit
	);

	/**
	 * @brief 컴포넌트가 특정 위치로 이동했을 때 충돌하는 모든 컴포넌트 반환 (실제 이동 없이 테스트만)
	 * @param Component 테스트할 컴포넌트
	 * @param TargetLocation 이동할 목표 위치 (world space)
	 * @param OutOverlappingComponents 충돌이 감지된 모든 컴포넌트 배열 (출력)
	 * @return 하나 이상의 충돌이 감지되면 true
	 * @note 이동 전 미리 호출하여 모든 충돌 대상을 확인 (예: 범위 공격, 다중 상호작용)
	 */
	bool SweepComponentMulti(
		UPrimitiveComponent* Component,
		const FVector& TargetLocation,
		TArray<UPrimitiveComponent*>& OutOverlappingComponents
	);

	bool LineTraceSingle(const FVector& Start, const FVector& End, FHitResult& OutHit, AActor* IgnoredActor, ECollisionTag IgnoredTag);

	/**
	 * @brief 액터가 특정 위치로 이동했을 때 첫 번째로 충돌하는 컴포넌트 반환 (실제 이동 없이 테스트만)
	 * @param Actor 테스트할 액터 (모든 PrimitiveComponent 검사)
	 * @param TargetLocation 이동할 목표 위치 (world space)
	 * @param OutHit 충돌 정보 (충돌한 컴포넌트, 액터 등)
	 * @param FilterTag 필터링할 CollisionTag (None이면 모든 태그 허용)
	 * @return 충돌이 감지되면 true
	 * @note Actor의 모든 PrimitiveComponent를 기준으로 sweep 테스트 수행 (가장 가까운 충돌 반환)
	 */
	bool SweepActorSingle(
		AActor* Actor,
		const FVector& TargetLocation,
		struct FHitResult& OutHit,
		ECollisionTag FilterTag = ECollisionTag::None
	);

	/**
	 * @brief 액터가 특정 위치로 이동했을 때 충돌하는 모든 컴포넌트 반환 (실제 이동 없이 테스트만)
	 * @param Actor 테스트할 액터 (모든 PrimitiveComponent 검사)
	 * @param TargetLocation 이동할 목표 위치 (world space)
	 * @param OutOverlappingComponents 충돌이 감지된 모든 컴포넌트 배열 (출력)
	 * @param FilterTag 필터링할 CollisionTag (None이면 모든 태그 허용)
	 * @return 하나 이상의 충돌이 감지되면 true
	 * @note Actor의 모든 PrimitiveComponent를 기준으로 sweep 테스트 수행
	 */
	bool SweepActorMulti(
		AActor* Actor,
		const FVector& TargetLocation,
		TArray<UPrimitiveComponent*>& OutOverlappingComponents,
		ECollisionTag FilterTag = ECollisionTag::None
	);

private:
	/**
	 * @brief 이전 프레임의 overlap 상태 추적 (변화 감지용)
	 * @note Key: 정렬된 component pair (A < B 기준, WeakObjectPtr로 댕글링 포인터 방지)
	 *       Value: overlap 상태 (true = overlapping, false = not overlapping)
	 */
	TMap<TPair<TWeakObjectPtr<UPrimitiveComponent>, TWeakObjectPtr<UPrimitiveComponent>>, bool, PairHash> PreviousOverlapState;
};
