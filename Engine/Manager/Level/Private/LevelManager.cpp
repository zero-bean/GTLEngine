#include "pch.h"
#include "Manager/Level/Public/LevelManager.h"

#include "Level/Public/Level.h"

IMPLEMENT_SINGLETON(ULevelManager)

ULevelManager::ULevelManager() = default;

ULevelManager::~ULevelManager() = default;

void ULevelManager::RegisterLevel(const wstring& InName, ULevel* InLevel)
{
	Levels[InName] = InLevel;
	if (!CurrentLevel)
	{
		CurrentLevel = InLevel;
	}
}

void ULevelManager::LoadLevel(const wstring& InName)
{
	if (Levels.find(InName) == Levels.end())
	{
		assert(!R"(Load할 레벨을 탐색하지 못함)");
	}

	CurrentLevel->Cleanup();

	CurrentLevel = Levels[InName];
	CurrentLevel->Init();
}

void ULevelManager::Update()
{
	CurrentLevel->Update();
}






