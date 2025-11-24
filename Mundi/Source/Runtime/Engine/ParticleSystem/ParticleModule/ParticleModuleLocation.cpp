#include "pch.h"
#include "ParticleModuleLocation.h"
#include "ParticleEmitterInstances.h"

// UParticleModuleLocation은 업데이트가 필요 없음, 생성될 좌표만 결정
void UParticleModuleLocation::Spawn(const FSpawnContext& SpawnContext)
{
	FVector SpawnLocationVector = SpawnLocation.GetValue(SpawnContext.Owner->EmitterTime);
	// 이미터의 현재 위치의 상대좌표라서 더함.
	SpawnContext.ParticleBase->Location += SpawnLocationVector;
}
