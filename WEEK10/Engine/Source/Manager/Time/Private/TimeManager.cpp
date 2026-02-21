#include "pch.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Utility/Public/ScopeCycleCounter.h"

IMPLEMENT_SINGLETON_CLASS(UTimeManager, UObject)

UTimeManager::UTimeManager()
{
	Initialize();
}

UTimeManager::~UTimeManager() = default;

void UTimeManager::Initialize()
{
	GameTime = 0.0f;
	DeltaTime = 0.0f;
}

void UTimeManager::Update()
{
	GameTime += DeltaTime;
}
