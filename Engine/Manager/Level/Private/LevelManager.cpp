#include "pch.h"
#include "Manager/Level/Public/LevelManager.h"

#include "Level/Public/Level.h"
#include "Mesh/Public/CubeActor.h"
#include "Mesh/Public/SphereActor.h"

IMPLEMENT_SINGLETON(ULevelManager)

ULevelManager::ULevelManager() = default;

ULevelManager::~ULevelManager()
{
	for (auto& Pair : Levels)
	{
		delete Pair.second;
	}
}

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

	if (CurrentLevel)
		CurrentLevel->Cleanup();

	CurrentLevel = Levels[InName];

	CurrentLevel->Init();
}

/**
 * @brief Test Code
 */
void ULevelManager::CreateDefaultLevel()
{
	Levels[L"Default"] = new ULevel();
	LoadLevel(L"Default");

	ASphereActor* Sphere1 = CurrentLevel->SpawnActor<ASphereActor>();
	Sphere1->SetActorLocation({1,0,0});

	ACubeActor* Cube1 = CurrentLevel->SpawnActor<ACubeActor>();
	Cube1->SetActorLocation({1,0,0});
	CurrentLevel->SetSelectedActor(Cube1);

	ACubeActor* Cube2 = CurrentLevel->SpawnActor<ACubeActor>();
	Cube1->SetActorLocation({0,1,0});
}

void ULevelManager::Update()
{
	if (CurrentLevel)
	{
		CurrentLevel->Update();
	}
}






