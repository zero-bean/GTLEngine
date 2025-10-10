#include "pch.h"
#include "Level/Public/Level.h"

#include "Actor/Public/Actor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Core/Public/Object.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/EditorEngine.h"
#include "Editor/Public/FrustumCull.h"
#include "Editor/Public/Viewport.h"
#include "Factory/Public/NewObject.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Utility/Public/ActorTypeMapper.h"
#include "Utility/Public/JsonSerializer.h"

#include <json.hpp>

#include "Core/Public/BVHierarchy.h"
#include "World/Public/World.h"

IMPLEMENT_CLASS(ULevel, UObject)

ULevel::ULevel() = default;

ULevel::ULevel(const FName& InName)
	: UObject(InName), Frustum(NewObject<FFrustumCull>())
{
}

void ULevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		// NOTE: Version 사용하지 않음
		//uint32 Version = 0;
		//FJsonSerializer::ReadUint32(InOutHandle, "Version", Version);

		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		uint32 NextUUID = 0;
		FJsonSerializer::ReadUint32(InOutHandle, "NextUUID", NextUUID);

		JSON PerspectiveCameraData;
		if (FJsonSerializer::ReadObject(InOutHandle, "PerspectiveCamera", PerspectiveCameraData))
		{
			UConfigManager::GetInstance().SetCameraSettingsFromJson(PerspectiveCameraData);
			URenderer::GetInstance().GetViewportClient()->ApplyAllCameraDataToViewportClients();
		}

		JSON PrimitivesJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "Primitives", PrimitivesJson))
		{
			// ObjectRange()를 사용하여 Primitives 객체의 모든 키-값 쌍을 순회
			for (auto& Pair : PrimitivesJson.ObjectRange())
			{
				// Pair.first는 ID 문자열, Pair.second는 단일 프리미티브의 JSON 데이터입니다.
				const FString& IdString = Pair.first;
				JSON& PrimitiveDataJson = Pair.second;

				FString TypeString;
				FJsonSerializer::ReadString(PrimitiveDataJson, "Type", TypeString);

				UClass* NewClass = FActorTypeMapper::TypeToActor(TypeString);

				AActor* NewActor = SpawnActorToLevel(NewClass, IdString);
				if (NewActor)
				{
					NewActor->Serialize(bInIsLoading, PrimitiveDataJson);
				}
			}
		}
	}

	// 저장
	else
	{
		// NOTE: Version 사용하지 않음
		//InOutHandle["Version"] = 1;

		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		InOutHandle["NextUUID"] = 0;

		// GetCameraSetting 호출 전에 뷰포트 클라이언트의 최신 데이터를 ConfigManager로 동기화합니다.
		URenderer::GetInstance().GetViewportClient()->UpdateCameraSettingsToConfig();
		InOutHandle["PerspectiveCamera"] = UConfigManager::GetInstance().GetCameraSettingsAsJson();

		JSON PrimitivesJson = json::Object();
		for (const TObjectPtr<AActor>& Actor : Actors)
		{
			JSON PrimitiveJson;
			PrimitiveJson["Type"] = FActorTypeMapper::ActorToType(Actor->GetClass());;
			Actor->Serialize(bInIsLoading, PrimitiveJson);

			PrimitivesJson[std::to_string(Actor->GetUUID())] = PrimitiveJson;
		}
		InOutHandle["Primitives"] = PrimitivesJson;
	}
}

UObject* ULevel::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<ULevel*>(Super::Duplicate(Parameters));

	DupObject->Frustum = Frustum;
	DupObject->ShowFlags = ShowFlags;
	DupObject->LODUpdateFrameCounter = LODUpdateFrameCounter;
	//DupObject->SelectedActor = SelectedActor;

	// @todo ActorsToDelete는 복제할 필요가 존재하는지 확인

	if (Frustum)
	{
		if (auto It = Parameters.DuplicationSeed.find(Frustum); It != Parameters.DuplicationSeed.end())
		{
			DupObject->Frustum = static_cast<FFrustumCull*>(It->second);
		}
		else
		{
			auto Params = InitStaticDuplicateObjectParams(Frustum, DupObject, FName::GetNone(), Parameters.DuplicationSeed, Parameters.CreatedObjects);
			auto DupFrustum = static_cast<FFrustumCull*>(Frustum->Duplicate(Params));

			DupObject->Frustum = DupFrustum;
		}
	}

	if (OwningWorld)
	{
		if (auto It = Parameters.DuplicationSeed.find(OwningWorld); It != Parameters.DuplicationSeed.end())
		{
			DupObject->OwningWorld = static_cast<UWorld*>(Parameters.DestOuter);
			//DupObject->OwningWorld = static_cast<UWorld*>(It->second);
		}
		else
		{
			OwningWorld = nullptr;
			UE_LOG_ERROR("Level의 생성 이전에 World가 생성되어야 합니다.");
		}
	}

	for (auto& Actor : Actors)
	{
		if (auto It = Parameters.DuplicationSeed.find(Actor); It != Parameters.DuplicationSeed.end())
		{
			DupObject->Actors.emplace_back(static_cast<AActor*>(It->second));
		}
		else
		{
			auto Params = InitStaticDuplicateObjectParams(Actor, DupObject, FName::GetNone(), Parameters.DuplicationSeed, Parameters.CreatedObjects);
			auto DupActor = static_cast<AActor*>(Actor->Duplicate(Params));
			DupObject->Actors.emplace_back(DupActor);
		}
	}

	for (auto& Component : LevelPrimitiveComponents)
	{
		if (auto It = Parameters.DuplicationSeed.find(Component); It != Parameters.DuplicationSeed.end())
		{
			DupObject->LevelPrimitiveComponents.emplace_back(static_cast<UPrimitiveComponent*>(It->second));
		}
		else
		{
			UE_LOG_ERROR("Actor에 포함되지 않는 Primitive가 발견되었습니다.");
		}
	}

	return DupObject;
}

void ULevel::Init()
{
	// TEST CODE
}

void ULevel::Tick(float DeltaSeconds)
{
	// Process Delayed Task
	ProcessPendingDeletions();

	uint64 AllocatedByte = GetAllocatedBytes();
	uint32 AllocatedCount = GetAllocatedCount();

	// UE_LOG("OwningWorld->GetWorldType() : %s", to_string(OwningWorld->GetWorldType()).data());
	// InitializeActorsInLevel();

	if (OwningWorld->GetWorldType() == EWorldType::Editor)
	{
		// UE_LOG("In Editor");
		for (auto& Actor : Actors)
		{
			if (Actor && Actor->IsActorTickEnabled() && Actor->IsTickInEditor())
			{
				Actor->Tick(DeltaSeconds);
			}
		}
	}
	else if (OwningWorld->GetWorldType() == EWorldType::PIE)
	{
		// UE_LOG("In PIE");
		for (auto& Actor : Actors)
		{
			Actor->SetActorTickEnabled(true);
			if (Actor && Actor->IsActorTickEnabled())
			{
				Actor->Tick(DeltaSeconds);
			}
		}
	}


	TickLODUpdate(DeltaSeconds);
}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
	SetSelectedActor(nullptr);

	// 1. 지연 삭제 목록에 남아있는 액터들을 먼저 처리합니다.
	ProcessPendingDeletions();

	// 2. Actors 배열에 남아있는 모든 액터의 메모리를 해제합니다.
	for (const auto& Actor : Actors)
	{
		delete Actor;
	}
	Actors.clear();

	// 3. 모든 액터 객체가 삭제되었으므로, 포인터를 담고 있던 컨테이너들을 비웁니다.
	ActorsToDelete.clear();
	LevelPrimitiveComponents.clear();

	// 4. 선택된 액터 참조를 안전하게 해제합니다.
	SelectedActor = nullptr;

	SafeDelete(Frustum);
}

void ULevel::InitializeActorsInLevel()
{
	LevelPrimitiveComponents.clear();
	for (auto& Actor : Actors)
	{
		if (Actor)
		{
			AddLevelPrimitiveComponentsInActor(Actor);
		}
	}

	TArray<FBVHPrimitive> BVHPrimitives;
	UBVHierarchy::GetInstance().ConvertComponentsToBVHPrimitives(LevelPrimitiveComponents, BVHPrimitives);
	UBVHierarchy::GetInstance().Build(BVHPrimitives);
}

AActor* ULevel::SpawnActorToLevel(UClass* InActorClass, const FName& InName)
{
	if (!InActorClass)
	{
		return nullptr;
	}

	AActor* NewActor = NewObject<AActor>(nullptr, TObjectPtr(InActorClass), InName);

	if (!NewActor) return nullptr;

	if (InName != FName::GetNone())
	{
		NewActor->SetName(InName);
	}

	Actors.push_back(TObjectPtr(NewActor));
	NewActor->BeginPlay();

	if (this == GEngine->GetCurrentLevel())
	{
		AddLevelPrimitiveComponentsInActor(NewActor);
		TArray<FBVHPrimitive> BVHPrimitives;
		UBVHierarchy::GetInstance().ConvertComponentsToBVHPrimitives(LevelPrimitiveComponents, BVHPrimitives);
		UBVHierarchy::GetInstance().Build(BVHPrimitives);
	}

	return NewActor;
}

void ULevel::RegisterDuplicatedActor(AActor* NewActor)
{
	if (!NewActor) return;

	for (auto Comp : NewActor->GetOwnedComponents())
	{
		if (Comp == nullptr)
		{
			throw std::runtime_error("FAIL");
		}

		if (Comp->GetClass() == nullptr)
		{
			throw std::runtime_error("FAIL!");
		}
	}

	Actors.emplace_back(NewActor);

	if (this == GEngine->GetCurrentLevel())
	{
		AddLevelPrimitiveComponentsInActor(NewActor);
		TArray<FBVHPrimitive> BVHPrimitives;
		UBVHierarchy::GetInstance().ConvertComponentsToBVHPrimitives(LevelPrimitiveComponents, BVHPrimitives);
		UBVHierarchy::GetInstance().Build(BVHPrimitives);
	}
}

TArray<TObjectPtr<UPrimitiveComponent>> ULevel::GetVisiblePrimitiveComponents(UCamera* InCamera)
{
	TArray<TObjectPtr<UPrimitiveComponent>> VisibleComponents{};
	if (Frustum == nullptr || InCamera == nullptr)
	{
		return VisibleComponents;
	}

	Frustum->Update(InCamera);
	// UBV Tree를 순회하며 컬링
	UBVHierarchy::GetInstance().FrustumCull(*Frustum, VisibleComponents);

	// 선형탐색으로 컬링
	// for (auto& PrimitiveComponent : LevelPrimitiveComponents)
	// {
	// 	// 이미 보이지 않는 primitive는 컬링할 필요 X
	// 	// Primitive visibility는 toggle로 제어되는 중
	// 	if (PrimitiveComponent == nullptr || !PrimitiveComponent->IsVisible())
	//  	{
	//  		continue;
	//  	}
	//
	//  	FAABB TargetAABB{};
	//  	PrimitiveComponent->GetWorldAABB(TargetAABB.Min, TargetAABB.Max);
	//  	if (Frustum->IsInFrustum(TargetAABB) == EFrustumTestResult::Inside)
	//  	{
	//  		VisibleComponents.push_back(PrimitiveComponent);
	// 	}
	// }

	// 값으로 반환하지만 Return Value Optimize가 컴파일러 단계에서 이뤄짐
	// 성능 측정해볼 필요 있음
	return VisibleComponents;
}

void ULevel::AddLevelPrimitiveComponentsInActor(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		if (!(Component->GetComponentType() >= EComponentType::Primitive)) continue;

		TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);

		if (!PrimitiveComponent)
		{
			continue;
		}

		/* 3가지 경우 존재.
		1: primitive show flag가 꺼져 있으면, 도형, 빌보드 모두 렌더링 안함.
		2: primitive show flag가 켜져 있고, billboard show flag가 켜져 있으면, 도형, 빌보드 모두 렌더링
		3: primitive show flag가 켜져 있고, billboard show flag가 꺼져 있으면, 도형은 렌더링 하지만, 빌보드는 렌더링 안함. */
		// 빌보드는 무조건 피킹이 된 actor의 빌보드여야 렌더링 가능
		if (!PrimitiveComponent->IsVisible() && (ShowFlags & EEngineShowFlags::SF_Primitives))
		{
			continue; // 현재 컴포넌트만 스킵하고 다음 컴포넌트로 계속
		}
		if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::Decal && !(ShowFlags & EEngineShowFlags::SF_Decals))
		{
			continue;
		}
		if (PrimitiveComponent->GetPrimitiveType() != EPrimitiveType::TextRender)
		{
			LevelPrimitiveComponents.push_back(PrimitiveComponent);
		}
		else if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::TextRender)
		{
			if (ShowFlags & EEngineShowFlags::SF_BillboardText)
			{
				LevelPrimitiveComponents.push_back(PrimitiveComponent);
			}
		}
	}
}

void ULevel::AddLevelPrimitiveComponent(TObjectPtr<UPrimitiveComponent> InPrimitiveComponent)
{
	if (!InPrimitiveComponent)
	{
		return;
	}

	LevelPrimitiveComponents.push_back(InPrimitiveComponent);

	TArray<FBVHPrimitive> BVHPrimitives;
	UBVHierarchy::GetInstance().ConvertComponentsToBVHPrimitives(LevelPrimitiveComponents, BVHPrimitives);
	UBVHierarchy::GetInstance().Build(BVHPrimitives);
}

void ULevel::SetSelectedActor(AActor* InActor)
{
	// Set Selected Actor
	if (SelectedActor)
	{
		for (auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (!(Component->GetComponentType() >= EComponentType::Primitive))
			{
				continue;
			}

			TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
			if (PrimitiveComponent->IsVisible())
			{
				PrimitiveComponent->SetColor({0.f, 0.f, 0.f, 0.f});
			}
		}
	}

	if (InActor != SelectedActor)
	{
		UUIManager::GetInstance().OnSelectedActorChanged(InActor);
	}
	SelectedActor = InActor;

	if (!SelectedActor)
	{
		// 선택 해제 시에도 LevelPrimitiveComponents 재구성 (Billboard 제거)
		InitializeActorsInLevel();
		return;
	}

	for (auto& Component : SelectedActor->GetOwnedComponents())
	{
		if (!(Component->GetComponentType() >= EComponentType::Primitive)) continue;

		TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
		if (PrimitiveComponent->IsVisible())
		{
			PrimitiveComponent->SetColor({1.f, 0.8f, 0.2f, 0.4f});
		}
	}

	// 선택된 Actor의 Billboard를 LevelPrimitiveComponents에 추가하기 위해 재구성
	InitializeActorsInLevel();
}

// Level에서 Actor 제거하는 함수
bool ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return false;
	}

	// Actors 리스트에서 제거
	for (auto Iterator = Actors.begin(); Iterator != Actors.end(); ++Iterator)
	{
		if (*Iterator == InActor)
		{
			Actors.erase(Iterator);
			break;
		}
	}

	// Remove Actor Selection
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}

	// Remove
	delete InActor;

	UE_LOG("Level: Actor Destroyed Successfully");
	return true;
}

// 지연 삭제를 위한 마킹
void ULevel::MarkActorForDeletion(AActor* InActor)
{
	if (!InActor)
	{
		UE_LOG("Level: MarkActorForDeletion: InActor Is Null");
		return;
	}

	// 이미 삭제 대기 중인지 확인
	for (AActor* PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			UE_LOG("Level: Actor Already Marked For Deletion");
			return;
		}
	}

	// 삭제 대기 리스트에 추가
	ActorsToDelete.push_back(InActor);
	UE_LOG("Level: 다음 Tick에 Actor를 제거하기 위한 마킹 처리: %s", InActor->GetName().ToString().data());

	// 선택 해제는 바로 처리
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}
}

void ULevel::ProcessPendingDeletions()
{
	if (ActorsToDelete.empty())
	{
		return;
	}

	UE_LOG("Level: %zu개의 객체 지연 삭제 프로세스 처리 시작", ActorsToDelete.size());

	// 원본 배열을 복사하여 사용 (DestroyActor가 원본을 수정할 가능성에 대비)
	TArray<AActor*> ActorsToProcess = ActorsToDelete;
	ActorsToDelete.clear();

	for (AActor* ActorToDelete : ActorsToProcess)
	{
		if (ActorToDelete)
		{
			DestroyActor(ActorToDelete);
		}
	}

	UE_LOG("Level: 모든 지연 삭제 프로세스 완료");
}

void ULevel::TickLODUpdate(float DeltaSeconds)
{
	LODUpdateFrameCounter += DeltaSeconds;

	// 0.2초마다 LOD 업데이트 실행
	if (LODUpdateFrameCounter >= LOD_UPDATE_INTERVAL)
	{
		LODUpdateFrameCounter -= LOD_UPDATE_INTERVAL;
		UpdateLODForAllMeshes();
	}
}

void ULevel::UpdateLODForAllMeshes()
{
	// 카메라 위치 가져오기
	URenderer* RendererPtr = nullptr;
	try
	{
		RendererPtr = &URenderer::GetInstance();
	}
	catch (...)
	{
		return; // Renderer가 초기화되지 않은 경우
	}

	if (!RendererPtr) return;

	FViewport* Viewport = RendererPtr->GetViewportClient();
	if (!Viewport) return;

	UCamera* Camera = Viewport->GetActiveCamera();
	if (!Camera) return;

	FVector CameraPosition = Camera->GetLocation();

	// 레벨의 모든 StaticMeshComponent에 대해 LOD 업데이트
	for (const TObjectPtr<AActor>& ActorPtr : Actors)
	{
		AActor* Actor = ActorPtr.Get();
		if (Actor)
		{
			const TArray<TObjectPtr<UActorComponent>>& Components = Actor->GetOwnedComponents();
			for (const TObjectPtr<UActorComponent>& ComponentPtr : Components)
			{
				UActorComponent* Component = ComponentPtr.Get();
				if (UStaticMeshComponent* MeshComp = dynamic_cast<UStaticMeshComponent*>(Component))
				{
					if (MeshComp->IsLODEnabled() || MeshComp->IsForcedLODEnabled())
					{
						MeshComp->UpdateLODBasedOnDistance(CameraPosition);
					}
				}
			}
		}
	}
}

void ULevel::SetGraphicsQuality(int32 QualityLevel)
{
	// 0: 울트라 (LOD0만), 1: 높음 (자동 LOD), 2: 보통 (LOD1만), 3: 낮음 (LOD2만)

	// 레벨의 모든 StaticMeshComponent에 대해 그래픽 품질 설정 적용
	FVector CameraPosition;
	URenderer* RendererPtr = &URenderer::GetInstance();
	if (RendererPtr)
	{
		FViewport* Viewport = RendererPtr->GetViewportClient();
		if (Viewport)
		{
			UCamera* Camera = Viewport->GetActiveCamera();
			if (Camera)
			{
				CameraPosition = Camera->GetLocation();
			}
		}
	}

	for (const TObjectPtr<AActor>& ActorPtr : Actors)
	{
		AActor* Actor = ActorPtr.Get();
		if (Actor)
		{
			const TArray<TObjectPtr<UActorComponent>>& Components = Actor->GetOwnedComponents();
			for (const TObjectPtr<UActorComponent>& ComponentPtr : Components)
			{
				UActorComponent* Component = ComponentPtr.Get();
				if (UStaticMeshComponent* MeshComp = dynamic_cast<UStaticMeshComponent*>(Component))
				{
					switch (QualityLevel)
					{
					case 0: // 울트라 (LOD0 강제 고정)
						MeshComp->SetLODEnabled(false);
						MeshComp->SetForcedLODLevel(0);
						MeshComp->SetLODLevel(0);
						break;

					case 1: // 높음 (자동 LOD)
						MeshComp->SetLODEnabled(true);
						MeshComp->SetForcedLODLevel(-1);
						MeshComp->UpdateLODBasedOnDistance(CameraPosition);
						break;

					case 2: // 보통 (LOD1 강제 고정)
						MeshComp->SetLODEnabled(false);
						MeshComp->SetForcedLODLevel(1);
						MeshComp->SetLODLevel(1);
						break;

					case 3: // 낮음 (LOD2 강제 고정)
						MeshComp->SetLODEnabled(false);
						MeshComp->SetForcedLODLevel(2);
						MeshComp->SetLODLevel(2);
						break;
					}
				}
			}
		}
	}

	// 로그 출력
	switch (QualityLevel)
	{
	case 0:
		UE_LOG("Level: 그래픽 품질을 울트라로 설정 (LOD0 강제 고정)");
		break;
	case 1:
		UE_LOG("Level: 그래픽 품질을 높음으로 설정 (자동 LOD)");
		break;
	case 2:
		UE_LOG("Level: 그래픽 품질을 보통으로 설정 (LOD1 강제 고정)");
		break;
	case 3:
		UE_LOG("Level: 그래픽 품질을 낮음으로 설정 (LOD2 강제 고정)");
		break;
	}
}

void ULevel::SetGlobalLODEnabled(bool bEnabled)
{
	// 레벨의 모든 StaticMeshComponent에 대해 LOD 활성화/비활성화 설정
	for (const TObjectPtr<AActor>& ActorPtr : Actors)
	{
		AActor* Actor = ActorPtr.Get();
		if (Actor)
		{
			const TArray<TObjectPtr<UActorComponent>>& Components = Actor->GetOwnedComponents();
			for (const TObjectPtr<UActorComponent>& ComponentPtr : Components)
			{
				UActorComponent* Component = ComponentPtr.Get();
				if (UStaticMeshComponent* MeshComp = dynamic_cast<UStaticMeshComponent*>(Component))
				{
					MeshComp->SetLODEnabled(bEnabled);
				}
			}
		}
	}

	UE_LOG("Level: 전역 LOD %s", bEnabled ? "활성화됨" : "비활성화됨");
}

void ULevel::SetMinLODLevel(int32 MinLevel)
{
	// 레벨의 모든 StaticMeshComponent에 대해 최소 LOD 레벨 설정
	for (const TObjectPtr<AActor>& ActorPtr : Actors)
	{
		AActor* Actor = ActorPtr.Get();
		if (Actor)
		{
			const TArray<TObjectPtr<UActorComponent>>& Components = Actor->GetOwnedComponents();
			for (const TObjectPtr<UActorComponent>& ComponentPtr : Components)
			{
				UActorComponent* Component = ComponentPtr.Get();
				if (UStaticMeshComponent* MeshComp = dynamic_cast<UStaticMeshComponent*>(Component))
				{
					MeshComp->SetMinLODLevel(MinLevel);
				}
			}
		}
	}

	switch (MinLevel)
	{
	case 0:
		UE_LOG("Level: 모든 LOD 허용 (0,1,2)");
		break;
	case 1:
		UE_LOG("Level: LOD 0 금지, LOD 1,2만 허용");
		break;
	case 2:
		UE_LOG("Level: LOD 0,1 금지, LOD 2만 허용 (최저 품질)");
		break;
	}
}

void ULevel::SetLODDistance1(float Distance)
{
	// 레벨의 모든 StaticMeshComponent에 대해 LOD1 전환 거리 설정
	for (const TObjectPtr<AActor>& ActorPtr : Actors)
	{
		AActor* Actor = ActorPtr.Get();
		if (Actor)
		{
			const TArray<TObjectPtr<UActorComponent>>& Components = Actor->GetOwnedComponents();
			for (const TObjectPtr<UActorComponent>& ComponentPtr : Components)
			{
				UActorComponent* Component = ComponentPtr.Get();
				if (UStaticMeshComponent* MeshComp = dynamic_cast<UStaticMeshComponent*>(Component))
				{
					MeshComp->SetLODDistance1(Distance);
				}
			}
		}
	}

	UE_LOG("Level: LOD 1 거리를 %.1f로 설정", Distance);
}

void ULevel::SetLODDistance2(float Distance)
{
	// 레벨의 모든 StaticMeshComponent에 대해 LOD2 전환 거리 설정
	for (const TObjectPtr<AActor>& ActorPtr : Actors)
	{
		AActor* Actor = ActorPtr.Get();
		if (Actor)
		{
			const TArray<TObjectPtr<UActorComponent>>& Components = Actor->GetOwnedComponents();
			for (const TObjectPtr<UActorComponent>& ComponentPtr : Components)
			{
				UActorComponent* Component = ComponentPtr.Get();
				if (UStaticMeshComponent* MeshComp = dynamic_cast<UStaticMeshComponent*>(Component))
				{
					MeshComp->SetLODDistance2(Distance);
				}
			}
		}
	}

	UE_LOG("Level: LOD 2 거리를 %.1f로 설정", Distance);
}
