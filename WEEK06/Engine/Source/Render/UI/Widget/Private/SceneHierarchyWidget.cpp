#include "pch.h"
#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

#include "Editor/Public/EditorEngine.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Editor/Public/Camera.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/ViewportClient.h"
#include "Editor/Public/Viewport.h"
#include "Global/Quaternion.h"

USceneHierarchyWidget::USceneHierarchyWidget()
	: UWidget("Scene Hierarchy Widget")
{
}

USceneHierarchyWidget::~USceneHierarchyWidget() = default;

void USceneHierarchyWidget::Initialize()
{
	UE_LOG("SceneHierarchyWidget: Initialized");
	// 캐시 무효화 (다음에 필요할 때 업데이트됨)
	InvalidateCache();
}

void USceneHierarchyWidget::Update()
{
	// 카메라 애니메이션 업데이트
	if (bIsCameraAnimating)
	{
		UpdateCameraAnimation();
	}
}

void USceneHierarchyWidget::RenderWidget()
{

	ULevel* CurrentLevel = GEngine->GetCurrentLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	// 헤더 정보
	ImGui::Text("Level: %s", CurrentLevel->GetName().ToString().c_str());
	ImGui::Separator();

	// 검색창 렌더링
	RenderSearchBar();
	const TArray<TObjectPtr<AActor>>& LevelActors = CurrentLevel->GetActors();

	if (LevelActors.empty())
	{
		ImGui::TextUnformatted("No Actors in Level");
		return;
	}

	// 변경 감지 후 필요시에만 캐시 업데이트
	if (LastCachedLevel != CurrentLevel || HasActorListChanged(LevelActors))
	{
		UpdateCacheIfNeeded(LevelActors);
	}

	// 필터링 업데이트
	if (bNeedsFilterUpdate)
	{
		UE_LOG("SceneHierarchy: 필터 업데이트 실행 중...");
		UpdateFilteredActors(LevelActors);
		bNeedsFilterUpdate = false;
	}

	// Actor 개수 표시
	if (SearchFilter.empty())
	{
		ImGui::Text("Total Actors: %zu", LevelActors.size());
	}
	else
	{
		ImGui::Text("%d / %zu actors", static_cast<int32>(FilteredIndices.size()), LevelActors.size());
	}
	ImGui::Spacing();



	// 가상화된 Actor 리스트 렌더링
	if (ImGui::BeginChild("ActorList", ImVec2(0, 0), true))
	{
		if (SearchFilter.empty())
		{
			// 캐시된 AllIndices 사용 (매 프레임 생성 제거)
			RenderVirtualizedActorList(LevelActors, CachedAllIndices);
		}
		else
		{
			if (!FilteredIndices.empty())
			{
				// 필터링된 Actor들 가상화 렌더링
				RenderVirtualizedActorList(LevelActors, FilteredIndices);
			}
			else
			{
				// 검색 결과가 없으면 메시지 표시
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "검색 결과가 없습니다.");
			}
		}
	}
	ImGui::EndChild();
}

/**
 * @brief Actor 정보를 렌더링하는 헬퍼 함수
 * @param InActor 렌더링할 Actor
 * @param InIndex Actor의 인덱스
 */
void USceneHierarchyWidget::RenderActorInfo(TObjectPtr<AActor> InActor, int32 InIndex)
{
	// TODO(KHJ): 컴포넌트 정보 탐색을 위한 트리 노드를 작업 후 남겨두었음, 필요하다면 사용할 것

	if (!InActor)
	{
		return;
	}

	ImGui::PushID(InIndex);

	// 현재 선택된 Actor인지 확인
	ULevel* CurrentLevel = GEngine->GetCurrentLevel();
	bool bIsSelected = (CurrentLevel && CurrentLevel->GetSelectedActor() == InActor);

	// 선택된 Actor는 하이라이트
	if (bIsSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // 노란색
	}

	FName ActorName = InActor->GetName();
	FString ActorDisplayName = ActorName.ToString();

	// 캐시된 PrimitiveComponent 정보 사용 (Cast 연산 없음)
	bool bHasPrimitive = false;
	bool bAllVisible = true;

	// 캐시에서 데이터 가져오기
	auto it = ActorPrimitiveCache.find(InActor);
	if (it != ActorPrimitiveCache.end())
	{
		const FActorPrimitiveCache& Cache = it->second;
		bHasPrimitive = Cache.bHasPrimitive;

		if (bHasPrimitive && !Cache.PrimitiveComponents.empty())
		{
			// Visibility 체크
			for (const auto& PrimitiveComponent : Cache.PrimitiveComponents)
			{
				if (PrimitiveComponent && !PrimitiveComponent->IsVisible())
				{
					bAllVisible = false;
					break;
				}
			}
		}
	}

	// PrimitiveComponent가 있는 경우에만 Visibility 버튼 표시
	if (bHasPrimitive)
	{
		if (ImGui::SmallButton(bAllVisible ? "[O]" : "[X]"))
		{
			// 캐시된 PrimitiveComponent들의 Visibility 토글
			bool bNewVisibility = !bAllVisible;
			const FActorPrimitiveCache& Cache = it->second;
			for (const auto& PrimComp : Cache.PrimitiveComponents)
			{
				if (PrimComp)
				{
					PrimComp->SetVisibility(bNewVisibility);
				}
			}
			UE_LOG_INFO("SceneHierarchy: %s의 가시성이 %s로 변경되었습니다",
			            ActorName.ToString().data(),
			            bNewVisibility ? "Visible" : "Hidden");
		}
	}
	else
	{
		// PrimitiveComponent가 없는 경우 비활성화된 버튼 표시
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
		ImGui::SmallButton("[-]");
		ImGui::PopStyleVar();
	}

	// 이름 클릭 감지 (오른쪽)
	ImGui::SameLine();

	// 이름 변경 모드인지 확인
	if (RenamingActor == InActor)
	{
		// 이름 변경 입력창
		ImGui::PushItemWidth(-1.0f);
		bool bEnterPressed = ImGui::InputText("##Rename", RenameBuffer, sizeof(RenameBuffer),
		                                      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		ImGui::PopItemWidth();

		// Enter 키로 확인
		if (bEnterPressed)
		{
			FinishRenaming(true);
		}
		// ESC 키로 취소
		else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			FinishRenaming(false);
		}
		// 다른 곳 클릭으로 취소
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered())
		{
			FinishRenaming(false);
		}

		// 포커스 설정 (첫 렌더링에서만)
		if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive())
		{
			ImGui::SetKeyboardFocusHere(-1);
		}
	}
	else
	{
		// 일반 선택 모드
		bool bClicked = ImGui::Selectable(ActorDisplayName.data(), bIsSelected, ImGuiSelectableFlags_SpanAllColumns);

		if (bClicked)
		{
			double CurrentTime = ImGui::GetTime();

			// 이미 선택된 Actor를 2초 이내 다시 클릭한 경우
			if (bIsSelected && LastClickedActor == InActor &&
				(CurrentTime - LastClickTime) > RENAME_CLICK_DELAY &&
				(CurrentTime - LastClickTime) < 2.0f)
			{
				// 이름 변경 모드 시작
				StartRenaming(InActor);
			}
			else
			{
				// 일반 선택
				SelectActor(InActor, false);
			}

			LastClickTime = CurrentTime;
			LastClickedActor = InActor;
		}

		// 더블 클릭 감지: 카메라 이동 수행
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			SelectActor(InActor, true);
			// 더블클릭 시 이름변경 모드 비활성화
			FinishRenaming(false);
		}
	}

	if (bIsSelected)
	{
		ImGui::PopStyleColor();
	}

	ImGui::PopID();
}

/**
 * @brief Actor를 선택하는 함수
 * @param InActor 선택할 Actor
 * @param bInFocusCamera 카메라 포커싱 여부 (더블 클릭 시 true)
 */
void USceneHierarchyWidget::SelectActor(TObjectPtr<AActor> InActor, bool bInFocusCamera)
{
	TObjectPtr<ULevel> CurrentLevel = GEngine->GetCurrentLevel();
	if (CurrentLevel)
	{
		CurrentLevel->SetSelectedActor(InActor);
		UE_LOG("SceneHierarchy: %s를 선택했습니다", InActor->GetName().ToString().data());

		// 카메라 포커싱은 더블 클릭에서만 수행
		if (InActor && bInFocusCamera)
		{
			FocusOnActor(InActor);
			UE_LOG_SUCCESS("SceneHierarchy: %s에 카메라 포커싱 완료", InActor->GetName().ToString().data());
		}
	}
}

/**
 * @brief 카메라를 특정 Actor에 포커스하는 함수
 * @param InActor 포커스할 Actor
 */
void USceneHierarchyWidget::FocusOnActor(TObjectPtr<AActor> InActor)
{
	if (!InActor) { return; }

	FViewport* Viewport = URenderer::GetInstance().GetViewportClient();
	if (!Viewport) { return; }

	TObjectPtr<UPrimitiveComponent> Prim = nullptr;
	if (InActor->GetRootComponent() && InActor->GetRootComponent()->IsA(UPrimitiveComponent::StaticClass()))
	{
		Prim = Cast<UPrimitiveComponent>(InActor->GetRootComponent());
	}

	if (!Prim)
	{
		UE_LOG_WARNING("SceneHierarchy: 액터에서 바운드를 계산할 프리미티브 컴포넌트를 찾을 수 없습니다.");
		return;
	}

	const FMatrix& ActorWorldMatrix = InActor->GetRootComponent()->GetWorldTransformMatrix();
	FVector ActorForward = { ActorWorldMatrix.Data[0][0], ActorWorldMatrix.Data[0][1], ActorWorldMatrix.Data[0][2] };
	ActorForward.Normalize();

	FVector ComponentMin, ComponentMax;
	Prim->GetWorldAABB(ComponentMin, ComponentMax);
	const FVector Center = (ComponentMin + ComponentMax) * 0.5f;
	const FVector Size = ComponentMax - ComponentMin;
	const float BoundingRadius = Size.Length() * 0.5f;

	auto& Viewports = Viewport->GetViewports();
	const int32 ViewportCount = static_cast<int32>(Viewports.size());

	CameraStartLocation.resize(ViewportCount);
	CameraStartRotation.resize(ViewportCount);
	CameraTargetLocation.resize(ViewportCount);
	CameraTargetRotation.resize(ViewportCount);

	for (int32 i = 0; i < ViewportCount; ++i)
	{
		UCamera& Camera = Viewports[i].Camera;
		CameraStartLocation[i] = Camera.GetLocation();
		CameraStartRotation[i] = Camera.GetRotation();

		if (Camera.GetCameraType() == EViewportCameraType::Perspective)
		{
			const float FovY = Camera.GetFovY();
			const float HalfFovRadian = FVector::GetDegreeToRadian(FovY * 0.5f);
			const float Distance = (BoundingRadius / sinf(HalfFovRadian)) * 1.2f;

			CameraTargetLocation[i] = Center - ActorForward * Distance;

			FVector LookAtDir = (Center - CameraTargetLocation[i]);
			LookAtDir.Normalize();

			float Yaw = FVector::GetRadianToDegree(atan2f(LookAtDir.Y, LookAtDir.X));
			float Pitch = FVector::GetRadianToDegree(asinf(LookAtDir.Z));
			CameraTargetRotation[i] = FVector(0.f, Pitch, Yaw);
		}
		else // Orthographic
		{
			CameraTargetLocation[i] = Center;
		}
	}

	bIsCameraAnimating = true;
	CameraAnimationTime = 0.0f;
}

/**
 * @brief 카메라 애니메이션을 업데이트하는 함수
 * 선형 보간을 활용한 부드러운 움직임을 구현함
 */
void USceneHierarchyWidget::UpdateCameraAnimation()
{
	if (!bIsCameraAnimating) { return; }

	FViewport* Viewport = URenderer::GetInstance().GetViewportClient();
	if (!Viewport)
	{
		bIsCameraAnimating = false;
		return;
	}

	CameraAnimationTime += DT;
	float Progress = CameraAnimationTime / CAMERA_ANIMATION_DURATION;

	if (Progress >= 1.0f)
	{
		Progress = 1.0f;
		bIsCameraAnimating = false;
	}

	float SmoothProgress;
	if (Progress < 0.5f)
	{
		SmoothProgress = 8.0f * Progress * Progress * Progress * Progress;
	}
	else
	{
		float ProgressFromEnd = Progress - 1.0f;
		SmoothProgress = 1.0f - 8.0f * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd;
	}

	auto& Viewports = Viewport->GetViewports();
	for (int Index = 0; Index < Viewports.size(); ++Index)
	{
		UCamera& Camera = Viewports[Index].Camera;

		FVector CurrentLocation = CameraStartLocation[Index] + (CameraTargetLocation[Index] - CameraStartLocation[Index]) * SmoothProgress;
		Camera.SetLocation(CurrentLocation);
		Viewport->SetFocusPoint(CurrentLocation);

		if (Camera.GetCameraType() == EViewportCameraType::Perspective)
		{
			FVector CurrentRotation = CameraStartRotation[Index] + (CameraTargetRotation[Index] - CameraStartRotation[Index]) * SmoothProgress;
			Camera.SetRotation(CurrentRotation);
		}
	}

	if (!bIsCameraAnimating)
	{
		UE_LOG_SUCCESS("SceneHierarchy: 카메라 포커싱 애니메이션 완료");
	}
}

/**
 * @brief 검색창을 렌더링하는 함수
 */
void USceneHierarchyWidget::RenderSearchBar()
{
	// 검색 지우기 버튼
	if (ImGui::SmallButton("X"))
	{
		memset(SearchBuffer, 0, sizeof(SearchBuffer));
		SearchFilter.clear();
		bNeedsFilterUpdate = true;
	}

	// 검색창
	ImGui::SameLine();
	ImGui::PushItemWidth(-1.0f); // 나머지 너비 모두 사용
	bool bTextChanged = ImGui::InputTextWithHint("##Search", "검색...", SearchBuffer, sizeof(SearchBuffer));
	ImGui::PopItemWidth();

	// 검색어가 변경되면 필터 업데이트 플래그 설정
	if (bTextChanged)
	{
		FString NewSearchFilter = FString(SearchBuffer);
		if (NewSearchFilter != SearchFilter)
		{
			UE_LOG("SceneHierarchy: 검색어 변경: '%s' -> '%s'", SearchFilter.data(), NewSearchFilter.data());
			SearchFilter = NewSearchFilter;
			bNeedsFilterUpdate = true;
		}
	}
}

/**
 * @brief 필터링된 Actor 인덱스 리스트를 업데이트하는 함수
 * @param InLevelActors 레벨의 모든 Actor 리스트
 */
void USceneHierarchyWidget::UpdateFilteredActors(const TArray<TObjectPtr<AActor>>& InLevelActors)
{
	FilteredIndices.clear();

	if (SearchFilter.empty())
	{
		return; // 검색어가 없으면 모든 Actor 표시
	}

	// 검색 성능 최적화: 대소문자 변환을 한 번만 수행
	FString SearchLower = SearchFilter;
	std::transform(SearchLower.begin(), SearchLower.end(), SearchLower.begin(), ::tolower);

	// UE_LOG("SceneHierarchy: 검색어 = '%s', 변환된 검색어 = '%s'", SearchFilter.data(), SearchLower.data());
	// UE_LOG("SceneHierarchy: Level에 %zu개의 Actor가 있습니다", InLevelActors.size());

	for (int32 i = 0; i < InLevelActors.size(); ++i)
	{
		if (InLevelActors[i])
		{
			FString ActorName = InLevelActors[i]->GetName().ToString();
			bool bMatches = IsActorMatchingSearch(ActorName, SearchLower);
			// UE_LOG("SceneHierarchy: Actor[%d] = '%s', 매치 = %s", i, ActorName.c_str(), bMatches ? "Yes" : "No");

			if (bMatches)
			{
				FilteredIndices.push_back(i);
			}
		}
	}

	UE_LOG("SceneHierarchy: 필터링 결과: %zu개 찾음", FilteredIndices.size());
}

/**
 * @brief Actor 이름이 검색어와 일치하는지 확인
 * @param InActorName Actor 이름
 * @param InSearchTerm 검색어 (대소문자를 무시)
 * @return 일치하면 true
 */
bool USceneHierarchyWidget::IsActorMatchingSearch(const FString& InActorName, const FString& InSearchTerm)
{
	if (InSearchTerm.empty())
	{
		return true;
	}

	FString ActorNameLower = InActorName;
	std::transform(ActorNameLower.begin(), ActorNameLower.end(), ActorNameLower.begin(), ::tolower);

	bool bResult = ActorNameLower.find(InSearchTerm) != std::string::npos;

	return bResult;
}

/**
 * @brief 이름 변경 모드를 시작하는 함수
 * @param InActor 이름을 변경할 Actor
 */
void USceneHierarchyWidget::StartRenaming(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return;
	}

	RenamingActor = InActor;
	FString CurrentName = InActor->GetName().ToString();

	// 현재 이름을 버퍼에 복사 - Detail 패널과 동일한 방식 사용
	strncpy_s(RenameBuffer, CurrentName.data(), sizeof(RenameBuffer) - 1);
	RenameBuffer[sizeof(RenameBuffer) - 1] = '\0';

	UE_LOG("SceneHierarchy: '%s' 에 대한 이름 변경 시작", CurrentName.data());
}

/**
 * @brief 이름 변경을 완료하는 함수
 * @param bInConfirm true면 적용, false면 취소
 */
void USceneHierarchyWidget::FinishRenaming(bool bInConfirm)
{
	if (!RenamingActor)
	{
		return;
	}

	if (bInConfirm)
	{
		FString NewName = FString(RenameBuffer);
		// 빈 이름 방지 및 이름 변경 여부 확인
		if (!NewName.empty() && NewName != RenamingActor->GetName().ToString())
		{
			// Detail 패널과 동일한 방식 사용
			RenamingActor->SetDisplayName(NewName);
			UE_LOG_SUCCESS("SceneHierarchy: Actor의 이름을 '%s' (으)로 변경하였습니다", NewName.c_str());

			// 검색 필터를 업데이트해야 할 수도 있음
			bNeedsFilterUpdate = true;
		}
		else if (NewName.empty())
		{
			UE_LOG_WARNING("SceneHierarchy: 빈 이름으로 인해 이름 변경 취소됨");
		}
	}
	else
	{
		UE_LOG_WARNING("SceneHierarchy: 이름 변경 취소");
	}

	// 상태 초기화
	RenamingActor = nullptr;
	RenameBuffer[0] = '\0';
}

/**
 * @brief Actor 리스트가 변경되었는지 확인하는 함수
 * @param InLevelActors 현재 레벨의 Actor 배열
 * @return 변경되었으면 true
 */
bool USceneHierarchyWidget::HasActorListChanged(const TArray<TObjectPtr<AActor>>& InLevelActors) const
{
	// 개수가 다르면 변경됨
	if (InLevelActors.size() != LastActorCount)
	{
		return true;
	}

	// 개수가 같으면 포인터 비교 (빠른 체크)
	if (InLevelActors.size() != LastActorList.size())
	{
		return true;
	}

	// 포인터 값들이 같은지 체크
	for (size_t i = 0; i < InLevelActors.size(); ++i)
	{
		if (InLevelActors[i] != LastActorList[i])
		{
			return true;
		}
	}

	return false; // 변경 없음
}

/**
 * @brief 변경이 감지되었을 때만 캐시를 업데이트하는 함수
 * @param InLevelActors 현재 레벨의 Actor 배열
 */
void USceneHierarchyWidget::UpdateCacheIfNeeded(const TArray<TObjectPtr<AActor>>& InLevelActors) const
{
	ULevel* CurrentLevel = GEngine->GetCurrentLevel();

	UE_LOG("SceneHierarchy: Cast 캐시 업데이트 시작...");

	// 캐시 초기화
	ActorPrimitiveCache.clear();

	// 모든 Actor에 대해 PrimitiveComponent 검색 및 캐싱 (Cast는 여기서만 발생)
	for (TObjectPtr<AActor> Actor : InLevelActors)
	{
		if (!Actor) continue;

		FActorPrimitiveCache Cache;

		// Actor의 모든 Component를 순회하며 PrimitiveComponent 찾기
		for (auto& Component : Actor->GetOwnedComponents())
		{
			if (TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
			{
				Cache.PrimitiveComponents.push_back(PrimitiveComponent);
				Cache.bHasPrimitive = true;
			}
		}

		// 캐시에 저장
		ActorPrimitiveCache[Actor] = std::move(Cache);
	}

	// AllIndices 배열도 함께 캐시 (한 번만 생성)
	CachedAllIndices.clear();
	CachedAllIndices.reserve(InLevelActors.size());
	for (int32 i = 0; i < InLevelActors.size(); ++i)
	{
		CachedAllIndices.push_back(i);
	}

	// 변경 감지를 위한 데이터 업데이트
	LastCachedLevel = CurrentLevel;
	LastActorCount = InLevelActors.size();
	LastActorList = InLevelActors; // 포인터 리스트 복사

	UE_LOG_SUCCESS("SceneHierarchy: Cast 캐시 업데이트 완료 (%zu actors)", InLevelActors.size());
}

/**
 * @brief 보이는 범위를 계산하는 함수
 * @param TotalItems 전체 항목 개수
 * @param ChildHeight 스크롤 영역 높이
 * @param OutStartIndex 시작 인덱스
 * @param OutEndIndex 끝 인덱스
 */
void USceneHierarchyWidget::CalculateVisibleRange(int32 TotalItems, float ChildHeight, int32& OutStartIndex, int32& OutEndIndex) const
{
	if (TotalItems == 0)
	{
		OutStartIndex = 0;
		OutEndIndex = 0;
		return;
	}

	// 현재 스크롤 위치 가져오기
	float ScrollY = ImGui::GetScrollY();

	// 보이는 영역에 해당하는 항목 범위 계산
	int32 FirstVisibleItem = static_cast<int32>(ScrollY / ITEM_HEIGHT);
	int32 LastVisibleItem = static_cast<int32>((ScrollY + ChildHeight) / ITEM_HEIGHT) + 1;

	// 버퍼 추가 (부드러운 스크롤링을 위해)
	OutStartIndex = std::max(0, FirstVisibleItem - BUFFER_ITEMS);
	OutEndIndex = std::min(TotalItems, LastVisibleItem + BUFFER_ITEMS);
}

/**
 * @brief 가상화된 Actor 리스트 렌더링 함수
 * @param InActors Actor 배열
 * @param InIndices 렌더링할 인덱스 배열
 */
void USceneHierarchyWidget::RenderVirtualizedActorList(const TArray<TObjectPtr<AActor>>& InActors, const TArray<int32>& InIndices)
{
	if (InIndices.empty()) return;

	// 전체 리스트 높이 계산
	float TotalHeight = static_cast<float>(InIndices.size()) * ITEM_HEIGHT;

	// 현재 Child 영역 높이
	float ChildHeight = ImGui::GetContentRegionAvail().y;

	// 보이는 범위 계산
	int32 StartIndex, EndIndex;
	CalculateVisibleRange(static_cast<int32>(InIndices.size()), ChildHeight, StartIndex, EndIndex);

	// 위쪽 여백 (보이지 않는 위쪽 항목들을 위한 공간)
	float TopSpacing = static_cast<float>(StartIndex) * ITEM_HEIGHT;
	if (TopSpacing > 0.0f)
	{
		ImGui::Dummy(ImVec2(0.0f, TopSpacing));
	}

	// 실제로 보이는 항목들만 렌더링
	for (int32 i = StartIndex; i < EndIndex; ++i)
	{
		if (i >= 0 && i < InIndices.size())
		{
			int32 ActorIndex = InIndices[i];
			if (ActorIndex >= 0 && ActorIndex < InActors.size() && InActors[ActorIndex])
			{
				RenderActorInfo(InActors[ActorIndex], ActorIndex);
			}
		}
	}

	// 아래쪽 여백 (보이지 않는 아래쪽 항목들을 위한 공간)
	float BottomSpacing = (static_cast<float>(InIndices.size()) - static_cast<float>(EndIndex)) * ITEM_HEIGHT;
	if (BottomSpacing > 0.0f)
	{
		ImGui::Dummy(ImVec2(0.0f, BottomSpacing));
	}
}

