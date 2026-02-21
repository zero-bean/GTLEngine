#pragma once
#include "Widget.h"

class AActor;
class ULevel;
class UCamera;

/**
 * @brief 현재 Level의 모든 Actor들을 트리 형태로 표시하는 Widget
 * Actor를 클릭하면 Level에서 선택되도록 하는 기능 포함
 */
class USceneHierarchyWidget :
	public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Special Member Function
	USceneHierarchyWidget();
	~USceneHierarchyWidget() override;

private:
	// UI 상태
	bool bShowDetails = true;

	// 검색 기능
	char SearchBuffer[256] = "";
	FString SearchFilter;
	TArray<int32> FilteredIndices; // 필터링된 Actor 인덱스 캐시
	bool bNeedsFilterUpdate = true; // 필터 업데이트 필요 여부

	// 가상화 렌더링
	static constexpr float ITEM_HEIGHT = 20.0f; // 각 Actor 항목의 높이
	static constexpr int32 BUFFER_ITEMS = 5; // 위아래 버퍼 항목 개수
	mutable TArray<int32> CachedAllIndices; // 전체 인덱스 배열 캐시

	// 이름 변경 기능
	TObjectPtr<AActor> RenamingActor = nullptr;
	char RenameBuffer[256] = "";
	double LastClickTime = 0.0f;
	TObjectPtr<AActor> LastClickedActor = nullptr;
	static constexpr float RENAME_CLICK_DELAY = 0.5f; // 두 번째 클릭 간격

	// Camera focus animation
	bool bIsCameraAnimating = false;
	float CameraAnimationTime = 0.0f;
	TArray<FVector> CameraStartLocation;
	TArray<FVector> CameraStartRotation;
	TArray<FVector> CameraTargetLocation;
	TArray<FVector> CameraTargetRotation;

	// Heuristic constant
	static constexpr float CAMERA_ANIMATION_DURATION = 0.8f;
	static constexpr float FOCUS_DISTANCE = 10.0f;

	// Camera movement
	void RenderActorInfo(TObjectPtr<AActor> InActor, int32 InIndex);
	void SelectActor(TObjectPtr<AActor> InActor, bool bInFocusCamera = false);
	void FocusOnActor(TObjectPtr<AActor> InActor);
	void UpdateCameraAnimation();

	// 검색 기능
	void RenderSearchBar();
	void UpdateFilteredActors(const TArray<TObjectPtr<AActor>>& InLevelActors);
	static bool IsActorMatchingSearch(const FString& InActorName, const FString& InSearchTerm);

	// 이름 변경 기능
	void StartRenaming(TObjectPtr<AActor> InActor);
	void FinishRenaming(bool bInConfirm);
	bool IsRenaming() const { return RenamingActor != nullptr; }

	// Cast 캐싱 - 더티 플래그 방식
	struct FActorPrimitiveCache
	{
		bool bHasPrimitive = false;
		TArray<TObjectPtr<UPrimitiveComponent>> PrimitiveComponents;
	};

	mutable TMap<TObjectPtr<AActor>, FActorPrimitiveCache> ActorPrimitiveCache;
	mutable TObjectPtr<ULevel> LastCachedLevel = nullptr; // 마지막으로 캐시한 레벨
	mutable size_t LastActorCount = 0; // 마지막으로 캐시한 Actor 개수
	mutable TArray<TObjectPtr<AActor>> LastActorList; // 마지막으로 캐시한 Actor 리스트 (포인터 비교용)

	void UpdateCacheIfNeeded(const TArray<TObjectPtr<AActor>>& InLevelActors) const;
	bool HasActorListChanged(const TArray<TObjectPtr<AActor>>& InLevelActors) const;
	void InvalidateCache() { ActorPrimitiveCache.clear(); LastCachedLevel = nullptr; LastActorCount = 0; LastActorList.clear(); CachedAllIndices.clear(); }

	// 가상화 헬퍼 함수들
	void RenderVirtualizedActorList(const TArray<TObjectPtr<AActor>>& InActors, const TArray<int32>& InIndices);
	void CalculateVisibleRange(int32 TotalItems, float ChildHeight, int32& OutStartIndex, int32& OutEndIndex) const;
};
