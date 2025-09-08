#include "pch.h"
#include "Level/Public/Level.h"

#include "Manager/Time/Public/TimeManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/Gizmo/Public/Gizmo.h"
#include "Render/AxisLine/Public/Axis.h"
#include "Render/UI/Window/Public/ActorInspectorWindow.h"

ULevel::ULevel()
{
	Gizmo = SpawnEditorActor<AGizmo>();
	Axis = SpawnEditorActor<AAxis>();
}

ULevel::ULevel(const wstring& InName)
	: Name(InName)
{
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
}

void ULevel::Init()
{
}

void ULevel::Update()
{
	Gizmo->SetTargetActor(SelectedActor);
	//여기서 해야할까요??
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

	// Set Inspector Actor

	UUIManager& UIManager = UUIManager::GetInstance();
	UActorInspectorWindow* InspectorWindow =
		reinterpret_cast<UActorInspectorWindow*>(UIManager.FindUIWindow("Actor Inspector"));
	InspectorWindow->SetSelectedActor(SelectedActor);
	
}
