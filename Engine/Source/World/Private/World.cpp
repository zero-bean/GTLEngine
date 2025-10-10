#include "pch.h"
#include "Editor/Public/FrustumCull.h"
#include "Level/Public/Level.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Utility/Public/JsonSerializer.h"
#include "World/Public/World.h"
#include <json.hpp>

using JSON = json::JSON;

IMPLEMENT_CLASS(UWorld, UObject)

UWorld::UWorld()
{
}

UWorld::~UWorld()
{
	SafeDelete(Level);
	SafeDelete(Frustum);
}

void UWorld::Tick(float DeltaSeconds)
{
	if (Level)
	{
		// UE_LOG("%s", to_string(WorldType).c_str());
		Level->Tick(DeltaSeconds);
	}
}

bool UWorld::LoadLevel(const FString& InFilePath)
{
	UE_LOG("World: Loading Level: %s", InFilePath.data());

	path FilePath(InFilePath);
	FString LevelName = FilePath.stem().string();

	ULevel* NewLevel = new ULevel(LevelName);
	NewLevel->SetOwningWorld(this);

	try
	{
		JSON LevelJsonData;
		if (FJsonSerializer::LoadJsonFromFile(LevelJsonData, InFilePath))
		{
			NewLevel->Serialize(true, LevelJsonData);
		}
		else
		{
			UE_LOG("World: Failed To Load Level From: %s", InFilePath.c_str());
			delete NewLevel;
			return false;
		}
	}
	catch (const exception& InException)
	{
		UE_LOG("World: Exception During Load: %s", InException.what());
		delete NewLevel;
		return false;
	}

	SwitchToLevel(NewLevel);
	UE_LOG("World: Level '%s' loaded successfully", LevelName.c_str());
	return true;
}

bool UWorld::SaveLevel(const FString& InFilePath)
{
	if (!Level)
	{
		UE_LOG("World: No Level To Save");
		return false;
	}

	path FilePath = InFilePath;
	if (FilePath.empty())
	{
		FilePath = GenerateLevelFilePath(Level->GetName() == FName::GetNone()
			                                 ? "Untitled"
			                                 : Level->GetName().ToString());
	}

	UE_LOG("World: Saving level to: %s", FilePath.string().c_str());

	try
	{
		JSON LevelJson;
		Level->Serialize(false, LevelJson);
		bool bSuccess = FJsonSerializer::SaveJsonToFile(LevelJson, FilePath.string());

		if (bSuccess)
		{
			UE_LOG("World: Level saved successfully");
		}
		else
		{
			UE_LOG("World: Failed to save level");
		}
		return bSuccess;
	}
	catch (const exception& Exception)
	{
		UE_LOG("World: Exception during save: %s", Exception.what());
		return false;
	}
}

bool UWorld::CreateNewLevel(const FString& InLevelName)
{
	ULevel* NewLevel = new ULevel(InLevelName);
	NewLevel->SetOwningWorld(this);
	SwitchToLevel(NewLevel);
	return true;
}

void UWorld::SetLevel(ULevel* InLevel)
{
	delete Level;
	Level = InLevel;
	if (Level)
	{
		Level->SetOwningWorld(this);
	}
}

void UWorld::SwitchToLevel(ULevel* InNewLevel)
{
	delete Level;
	Level = InNewLevel;
	if (Level)
	{
		Level->Init();
		Level->InitializeActorsInLevel();
		UE_LOG("World: Switched to Level '%s'", Level->GetName().ToString().c_str());
	}
	else
	{
		UE_LOG("World: Switched to null level");
	}
}

path UWorld::GetLevelDirectory()
{
	UPathManager& PathManager = UPathManager::GetInstance();
	return PathManager.GetWorldPath();
}

path UWorld::GenerateLevelFilePath(const FString& InLevelName)
{
	path LevelDirectory = GetLevelDirectory();
	path FileName = InLevelName + ".json";
	return LevelDirectory / FileName;
}

void UWorld::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);
}

UObject* UWorld::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UWorld*>(Super::Duplicate(Parameters));

	DupObject->WorldType = WorldType;

	if (Level)
	{
		if (auto It = Parameters.DuplicationSeed.find(Level); It != Parameters.DuplicationSeed.end())
		{
			DupObject->Level = static_cast<ULevel*>(It->second);
		}
		else
		{
			auto Params = InitStaticDuplicateObjectParams(Level, DupObject, FName::GetNone(), Parameters.DuplicationSeed, Parameters.CreatedObjects);
			auto DupLevel = static_cast<ULevel*>(Level->Duplicate(Params));

			DupLevel->SetName("Test");
			DupObject->Level = DupLevel;
		}
	}

	return DupObject;
}
