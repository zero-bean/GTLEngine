#include "pch.h"
#include "Level/Public/Level.h"

#include "Manager/Time/Public/TimeManager.h"
#include "Render/Gizmo/Public/Gizmo.h"

ULevel::ULevel()
{
	Gizmo = SpawnActor<AGizmo>();
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
	if (SelectedActor)
	{
		Gizmo->SetTargetActor(SelectedActor);
	}
	for (auto& Object : LevelActors)
	{
		AActor* Actor = dynamic_cast<AActor*>(Object);
		if (Actor)
		{
			Actor->Tick(UTimeManager::GetInstance().GetDeltaTime());
		}
	}
}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
}

void ULevel::AddPrimitiveComponent(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		UPrimitiveComponent* Comp = dynamic_cast<UPrimitiveComponent*>(Component);
		if (Comp)
		{
			PrimitiveComponents.push_back(Comp);
		}
	}
}
