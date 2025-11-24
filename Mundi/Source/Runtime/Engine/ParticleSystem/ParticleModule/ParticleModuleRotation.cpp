#include "pch.h"
#include "ParticleModuleRotation.h"
#include "ParticleModule.h"

void UParticleModuleRotation::Spawn(const FSpawnContext& SpawnContext)
{
	float StartRotationValue = StartRotation.GetValue(SpawnContext.SpawnTime, FMath::FRand());
	SpawnContext.ParticleBase->Rotation = StartRotationValue;
	SpawnContext.ParticleBase->RotationRate = StartRotationValue;
}
