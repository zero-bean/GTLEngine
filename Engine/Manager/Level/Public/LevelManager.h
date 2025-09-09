#pragma once
#include "Core/Public/Object.h"

class ULevel;

class ULevelManager :
	public UObject
{
DECLARE_SINGLETON(ULevelManager)

public:
	void Update() const;
	void CreateDefaultLevel();
	void RegisterLevel(const FString& InName, ULevel* InLevel);
	void LoadLevel(const FString& InName);
	void Shutdown();

	// Getter
	ULevel* GetCurrentLevel() const { return CurrentLevel; }

	// Save & Load System
	bool SaveCurrentLevel(const FString& InFilePath) const;
	bool LoadLevel(const FString& InLevelName, const FString& InFilePath);
	bool CreateNewLevel(const FString& InLevelName);
	path GetLevelDirectory() const;
	path GenerateLevelFilePath(const FString& InLevelName) const;

	// Metadata Conversion Functions
	struct FLevelMetadata ConvertLevelToMetadata(ULevel* InLevel) const;
	bool LoadLevelFromMetadata(ULevel* InLevel, const struct FLevelMetadata& InMetadata) const;

private:
	ULevel* CurrentLevel;
	TMap<FString, ULevel*> Levels;
};
