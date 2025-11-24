#include "pch.h"
#include "ParticleModuleVelocity.h"
#include "ParticleEmitterInstances.h"

void UParticleModuleVelocity::Spawn(const FSpawnContext& SpawnContext)
{
	FVector StartVelocityVector = StartVelocity.GetValue(SpawnContext.Owner->EmitterTime, FMath::FRand());
	// 이미터에 대한 상대속도
	SpawnContext.ParticleBase->BaseVelocity += StartVelocityVector;
	SpawnContext.ParticleBase->Velocity += StartVelocityVector;
}
