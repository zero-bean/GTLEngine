#pragma once
#include "Core/Public/Object.h"

using std::chrono::high_resolution_clock;

UCLASS()
class UTimeManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UTimeManager, UObject)

public:
	void Update();

	// Getter & Setter
	float GetFPS() const { return 1 / DeltaTime; }
	float GetRealDeltaTime() const { return DeltaTime; }
	float GetDeltaTime() const { return DeltaTime * GlobalTimeDilation; }
	float GetGameTime() const { return GameTime; }

	void SetDeltaTime(float InDeltaTime) { DeltaTime = InDeltaTime; }
	void SetGlobalTimeDilation(float InTimeDilation) { GlobalTimeDilation = InTimeDilation; }

private:
	void Initialize();

	float GameTime;
	float DeltaTime;
	float GlobalTimeDilation = 1.0f;
};
