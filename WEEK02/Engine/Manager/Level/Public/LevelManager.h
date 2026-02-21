#pragma once
#include "Core/Public/Object.h"

class ULevel;
struct FLevelMetadata;

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
	static path GetLevelDirectory();
	static path GenerateLevelFilePath(const FString& InLevelName);

	// Metadata Conversion Functions
	static FLevelMetadata ConvertLevelToMetadata(ULevel* InLevel);
	static bool LoadLevelFromMetadata(ULevel* InLevel, const FLevelMetadata& InMetadata);

private:
	ULevel* CurrentLevel;
	TMap<FString, ULevel*> Levels;
};
