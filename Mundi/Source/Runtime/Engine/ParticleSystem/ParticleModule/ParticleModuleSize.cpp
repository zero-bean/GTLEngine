#include "pch.h"
#include "ParticleModuleSize.h"
#include "ParticleEmitterInstances.h"

void UParticleModuleSize::Spawn(const FSpawnContext& SpawnContext)
{
	FVector StartSizeVector = StartSize.GetValue(SpawnContext.Owner->EmitterTime, FMath::FRand());

	SpawnContext.ParticleBase->Size = StartSizeVector;

}
