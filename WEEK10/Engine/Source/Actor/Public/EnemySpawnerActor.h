#pragma once
#include "Actor/Public/Actor.h"
#include "Core/Public/Delegate.h"

// Delegate 선언: FVector를 인자로 받음
DECLARE_DELEGATE(FOnEnemySpawnRequested);

/**
 * @brief Enemy 스폰 전용 Actor
 * - 자체 Delegate 보유
 * - Lua에서 Broadcast 가능한 API 제공
 */
class AEnemySpawnerActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(AEnemySpawnerActor, AActor)

public:
	AEnemySpawnerActor();

	/**
	 * @brief Enemy 스폰 요청 (Delegate Broadcast)
	 * @param SpawnLocation 스폰할 위치
	 */
	void RequestSpawn();

	/**
	 * @brief IDelegateProvider 구현
	 */
	virtual TArray<FDelegateInfoBase*> GetDelegates() const override;

public:
	// Delegate: 스폰 요청 이벤트
	FOnEnemySpawnRequested OnEnemySpawnRequested;
};
