#include "pch.h"
#include "Level/Public/Level.h"

#include "Manager/UI/Public/UIManager.h"
#include "Mesh/Public/Actor.h"

#include "Render/UI/Window/Public/CameraPanelWindow.h"

ULevel::ULevel()
{

}

ULevel::ULevel(const FString& InName)
	: UObject(InName)
{
}

ULevel::~ULevel()
{
	for (auto Actor : LevelActors)
	{
		SafeDelete(Actor);
	}

	//Deprecated : EditorPrimitive는 에디터에서 처리
	/*for (auto Actor : EditorActors)
	{
		SafeDelete(Actor);
	}*/

	SafeDelete(CameraPtr);
}

void ULevel::Init()
{
	// TEST CODE
}

void ULevel::Update()
{
	// Process Delayed Task
	ProcessPendingDeletions();

	LevelPrimitiveComponents.clear();
	//Deprecated : EditorPrimitive는 에디터에서 처리
	//EditorPrimitiveComponents.clear();

	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			Actor->Tick();
			AddLevelPrimitiveComponent(Actor);
		}
	}

	//Deprecated : EditorPrimitive는 에디터에서 처리
	/*for (auto& Actor : EditorActors)
	{
		if (Actor)
		{
			Actor->Tick();
			AddEditorPrimitiveComponent(Actor);
		}
	}*/

}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
}

void ULevel::AddLevelPrimitiveComponent(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		if (Component->GetComponentType() >= EComponentType::Primitive)
		{
			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
			if (PrimitiveComponent->IsVisible())
			{
				LevelPrimitiveComponents.push_back(PrimitiveComponent);
			}
		}
	}
}
//Deprecated : EditorPrimitive는 에디터에서 처리
//void ULevel::AddEditorPrimitiveComponent(AActor* Actor)
//{
//	if (!Actor) return;
//
//	for (auto& Component : Actor->GetOwnedComponents())
//	{
//		if (Component->GetComponentType() >= EComponentType::Primitive)
//		{
//			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
//			if (PrimitiveComponent->IsVisible())
//			{
//				EditorPrimitiveComponents.push_back(PrimitiveComponent);
//			}
//		}
//	}
//}

void ULevel::SetSelectedActor(AActor* InActor)
{
	// Set Selected Actor
	if (SelectedActor)
	{
		for (auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (Component->GetComponentType() >= EComponentType::Primitive)
			{
				UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
				if (PrimitiveComponent->IsVisible())
				{
					PrimitiveComponent->SetColor({0.f, 0.f, 0.f, 0.f});
				}
			}
		}
	}

	SelectedActor = InActor;
	if (SelectedActor)
	{
		for (auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (Component->GetComponentType() >= EComponentType::Primitive)
			{
				UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
				if (PrimitiveComponent->IsVisible())
				{
					PrimitiveComponent->SetColor({1.f, 0.8f, 0.2f, 0.4f});
				}
			}
		}
	}
	//Gizmo->SetTargetActor(SelectedActor);
}

/**
 * @brief Level에서 Actor 제거하는 함수
 */
bool ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return false;
	}

	// LevelActors 리스트에서 제거
	for (auto Iterator = LevelActors.begin(); Iterator != LevelActors.end(); ++Iterator)
	{
		if (*Iterator == InActor)
		{
			LevelActors.erase(Iterator);
			break;
		}
	}

	//Deprecated : EditorPrimitive는 에디터에서 처리
	// 필요하다면 EditorActors 리스트에서도 제거
	/*for (auto Iterator = EditorActors.begin(); Iterator != EditorActors.end(); ++Iterator)
	{
		if (*Iterator == InActor)
		{
			EditorActors.erase(Iterator);
			break;
		}
	}*/

	// Remove Actor Selection
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;

		//Deprecated : Gizmo는 에디터에서 처리
		// Gizmo Target Release
		/*if (Gizmo)
		{
			Gizmo->SetTargetActor(nullptr);
		}*/
	}

	// Remove
	delete InActor;

	UE_LOG("[Level] Actor Destroyed Successfully");
	return true;
}

/**
 * @brief Delete In Next Tick
 */
void ULevel::MarkActorForDeletion(AActor* InActor)
{
	if (!InActor)
	{
		UE_LOG("[Level] MarkActorForDeletion: InActor Is Null");
		return;
	}

	// 이미 삭제 대기 중인지 확인
	for (AActor* PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			UE_LOG("[Level] Actor Already Marked For Deletion");
			return;
		}
	}

	// 삭제 대기 리스트에 추가
	ActorsToDelete.push_back(InActor);
	UE_LOG("[Level] Actor Marked For Deletion In Next Tick: %p", InActor);

	// 선택 해제는 바로 처리
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;

		//Deprecated : Gizmo는 에디터에서 처리
		// Gizmo Target도 즉시 해제
		/*if (Gizmo)
		{
			Gizmo->SetTargetActor(nullptr);
		}*/
	}
}

/**
 * @brief Level에서 Actor를 실질적으로 제거하는 함수
 * 이전 Tick에서 마킹된 Actor를 제거한다
 */
void ULevel::ProcessPendingDeletions()
{
	if (ActorsToDelete.empty())
	{
		return;
	}

	UE_LOG("[Level] Processing %zu Pending Deletions", ActorsToDelete.size());

	// 대기 중인 액터들을 삭제
	for (AActor* ActorToDelete : ActorsToDelete)
	{
		if (!ActorToDelete)
			continue;

		// 혹시 남아있을 수 있는 참조 정리
		if (SelectedActor == ActorToDelete)
		{
			SelectedActor = nullptr;
			/*if (Gizmo)
			{
				Gizmo->SetTargetActor(nullptr);
			}*/
		}

		// LevelActors 리스트에서 제거
		for (auto Iterator = LevelActors.begin(); Iterator != LevelActors.end(); ++Iterator)
		{
			if (*Iterator == ActorToDelete)
			{
				LevelActors.erase(Iterator);
				break;
			}
		}

		//Deprecated : EditorActor는 에디터에서 처리
		// EditorActors 리스트에서도 제거
		/*for (auto Iterator = EditorActors.begin(); Iterator != EditorActors.end(); ++Iterator)
		{
			if (*Iterator == ActorToDelete)
			{
				EditorActors.erase(Iterator);
				break;
			}
		}*/

		// Release Memory
		delete ActorToDelete;
		UE_LOG("[Level] Actor Deleted: %p", ActorToDelete);
	}

	// Clear TArray
	ActorsToDelete.clear();
	UE_LOG("[Level] All Pending Deletions Processed");
}
