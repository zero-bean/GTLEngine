#include "pch.h"
#include "Actor/Public/EnemySpawnerActor.h"
#include "Component/Public/ScriptComponent.h"

IMPLEMENT_CLASS(AEnemySpawnerActor, AActor)

AEnemySpawnerActor::AEnemySpawnerActor()
	: AActor()
{
}

void AEnemySpawnerActor::RequestSpawn()
{

	// Delegate Broadcast
	OnEnemySpawnRequested.Broadcast();
}

TArray<FDelegateInfoBase*> AEnemySpawnerActor::GetDelegates() const
{
	// 부모의 Delegate들 가져오기
	TArray<FDelegateInfoBase*> Result = AActor::GetDelegates();

	// 자체 Delegate 추가
	AEnemySpawnerActor* MutableThis = const_cast<AEnemySpawnerActor*>(this);
	Result.Add(MakeDelegateInfo("OnEnemySpawnRequested",
		&MutableThis->OnEnemySpawnRequested));

	return Result;
}
