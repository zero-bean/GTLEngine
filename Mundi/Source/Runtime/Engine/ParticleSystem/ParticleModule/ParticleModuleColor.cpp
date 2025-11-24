#include "pch.h"
#include "ParticleModuleColor.h"
#include "ParticleEmitterInstances.h"

void UParticleModuleColor::Spawn(const FSpawnContext& SpawnContext)
{
	FVector StartColorVector = StartColor.GetValue(0.0f, FMath::FRand());
	SpawnContext.ParticleBase->Color = FLinearColor(StartColorVector);
}
