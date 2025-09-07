#include "pch.h"
#include "Level/Public/Level.h"

#include "Manager/Time/Public/TimeManager.h"
#include "Render/Gizmo/Public/Gizmo.h"

ULevel::ULevel()
{
	Gizmo = new AGizmo();
	AddActor(Gizmo);
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
