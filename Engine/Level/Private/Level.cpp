#include "pch.h"
#include "Level/Public/Level.h"

#include "Manager/UI/Public/UIManager.h"
#include "Render/Gizmo/Public/Gizmo.h"
#include "Render/AxisLine/Public/Axis.h"
#include "Render/Grid/Public/Grid.h"
#include "Render/Gizmo/Public/GizmoArrow.h"
#include "Camera/Public/Camera.h"
#include "Render/UI/Window/Public/CameraPanelWindow.h"
ULevel::ULevel()
{
	Gizmo = SpawnEditorActor<AGizmo>();
	Axis = SpawnEditorActor<AAxis>();
	Grid = SpawnEditorActor<AGrid>();
	CameraPtr = new Camera();
}

ULevel::ULevel(const FString& InName)
	: UObject(InName)
{
	Gizmo = SpawnEditorActor<AGizmo>();
	Axis = SpawnEditorActor<AAxis>();
	Grid = SpawnEditorActor<AGrid>();
	CameraPtr = new Camera();
}

ULevel::~ULevel()
{
	for (auto Actor : LevelActors)
	{
		SafeDelete(Actor);
	}

	for (auto Actor : EditorActors)
	{
		SafeDelete(Actor);
	}

	SafeDelete(CameraPtr);
}

void ULevel::Init()
{
	// TEST CODE
	if (UCameraPanelWindow* Window =
		dynamic_cast<UCameraPanelWindow*>(UUIManager::GetInstance().FindUIWindow("Camera Control")))
	{
		Window->SetCamera(CameraPtr);
	}
}

void ULevel::Update()
{
	// Process Delayed Task
	ProcessPendingDeletions();

	LevelPrimitiveComponents.clear();
	EditorPrimitiveComponents.clear();

	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			Actor->Tick();
			AddLevelPrimitiveComponent(Actor);
		}
	}
	for (auto& Actor : EditorActors)
	{
		if (Actor)
		{
			Actor->Tick();
			AddEditorPrimitiveComponent(Actor);
		}
	}

	if (CameraPtr)
	{
		CameraPtr->Update();
	}
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

void ULevel::AddEditorPrimitiveComponent(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		if (Component->GetComponentType() >= EComponentType::Primitive)
		{
			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
			if (PrimitiveComponent->IsVisible())
			{
				EditorPrimitiveComponents.push_back(PrimitiveComponent);
			}
		}
	}
}

void ULevel::SetSelectedActor(AActor* InActor)
{
	// Set Selected Actor
	SelectedActor = InActor;
	Gizmo->SetTargetActor(SelectedActor);
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

	// 필요하다면 EditorActors 리스트에서도 제거
	for (auto Iterator = EditorActors.begin(); Iterator != EditorActors.end(); ++Iterator)
	{
		if (*Iterator == InActor)
		{
			EditorActors.erase(Iterator);
			break;
		}
	}

	// Remove Actor Selection
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;

		// Gizmo Target Release
		if (Gizmo)
		{
			Gizmo->SetTargetActor(nullptr);
		}
	}

	// Remove
	delete InActor;

	cout << "[Level] Actor Destroyed Successfully" << "\n";
	return true;
}

/**
 * @brief Delete In Next Tick
 */
void ULevel::MarkActorForDeletion(AActor* InActor)
{
	if (!InActor)
	{
		cout << "[Level] MarkActorForDeletion: InActor Is Null" << "\n";
		return;
	}

	// 이미 삭제 대기 중인지 확인
	for (AActor* PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			cout << "[Level] Actor Already Marked For Deletion" << "\n";
			return;
		}
	}

	// 삭제 대기 리스트에 추가
	ActorsToDelete.push_back(InActor);
	cout << "[Level] Actor Marked For Deletion In Next Tick: " << InActor << "\n";

	// 선택 해제는 바로 처리
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;

		// Gizmo Target도 즉시 해제
		if (Gizmo)
		{
			Gizmo->SetTargetActor(nullptr);
		}
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

	cout << "[Level] Processing " << ActorsToDelete.size() << " Pending Deletions" << "\n";

	// 대기 중인 액터들을 삭제
	for (AActor* ActorToDelete : ActorsToDelete)
	{
		if (!ActorToDelete)
			continue;

		// 혹시 남아있을 수 있는 참조 정리
		if (SelectedActor == ActorToDelete)
		{
			SelectedActor = nullptr;
			if (Gizmo)
			{
				Gizmo->SetTargetActor(nullptr);
			}
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

		// EditorActors 리스트에서도 제거
		for (auto Iterator = EditorActors.begin(); Iterator != EditorActors.end(); ++Iterator)
		{
			if (*Iterator == ActorToDelete)
			{
				EditorActors.erase(Iterator);
				break;
			}
		}

		// Release Memory
		delete ActorToDelete;
		cout << "[Level] Actor Deleted: " << ActorToDelete << "\n";
	}

	// Clear TArray
	ActorsToDelete.clear();
	cout << "[Level] All Pending Deletions Processed" << "\n";
}
