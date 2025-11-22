#include "pch.h"
#include "ParticleModuleLifetime.h"
#include "Distribution.h"
#include "ParticleEmitterInstances.h"

// UParticleModuleLifetime은 업데이트가 필요 없음
void UParticleModuleLifetime::Spawn(const FSpawnContext& SpawnContext)
{
	float LifeTimeValue= LifeTime.GetValue(SpawnContext.Owner->EmitterTime, FMath::FRand());
	SpawnContext.ParticleBase->Lifetime = LifeTimeValue;
	SpawnContext.ParticleBase->RelativeTime = 0.0f;
}

