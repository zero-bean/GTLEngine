#pragma once
#include "Core/Public/Object.h"

class ULevel;

class ULevelManager :
	public UObject
{
DECLARE_SINGLETON(ULevelManager)

public:
	void Update();
	void RegisterLevel(const wstring& InName, ULevel* InLevel);
	void LoadLevel(const wstring& InName);

	// Getter
	ULevel* GetCurrentLevel() const { return CurrentLevel; }

	//test
	void CreateDefaultLevel();

private:
	ULevel* CurrentLevel;
	TMap<wstring, ULevel*> Levels;
};
