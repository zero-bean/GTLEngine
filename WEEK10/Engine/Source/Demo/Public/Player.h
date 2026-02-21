#pragma once
#include "Actor/Public/Actor.h"
#include "Core/Public/Delegate.h"

// Player 추적 정보 Delegate (LightLevel, PlayerLocation)
DECLARE_DELEGATE(FOnPlayerTracking, float, FVector);

class APlayer : public AActor
{
	DECLARE_CLASS(APlayer, AActor)

// Delegate Section
public:
	/** Enemy가 구독할 수 있는 추적 정보 Delegate */
	FOnPlayerTracking OnPlayerTracking;

	/** Lua에서 호출 가능한 Broadcast 헬퍼 함수 */
	void BroadcastTracking(float LightLevel, const FVector& PlayerLocation)
	{
		OnPlayerTracking.Broadcast(LightLevel, PlayerLocation);
	}

public:
	APlayer();

	UClass* GetDefaultRootComponent() override;
	void InitializeComponents() override;
};
