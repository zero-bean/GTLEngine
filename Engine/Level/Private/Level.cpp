#include "pch.h"
#include "Level/Public/Level.h"

#include "Manager/Time/Public/TimeManager.h"
#include "Render/Gizmo/Public/Gizmo.h"
#include "Render/AxisLine/Public/Axis.h"

ULevel::ULevel()
{
	Gizmo = SpawnEditorActor<AGizmo>();
	Axis = SpawnEditorActor<AAxis>();
}

ULevel::ULevel(const wstring& InName)
	: Name(InName)
{
	ULevel::ULevel();
}

ULevel::~ULevel()
{
	for (auto Actor : LevelActors)
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
			Actor->Tick(UTimeManager::GetInstance().GetDeltaTime());
			AddLevelPrimitiveComponent(Actor);
		}
	}
	for (auto& Actor : EditorActors)
	{
		if (Actor)
		{
			Actor->Tick(UTimeManager::GetInstance().GetDeltaTime());
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

	for (auto& Component: Actor->GetOwnedComponents())
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

	for (auto& Component: Actor->GetOwnedComponents())
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
