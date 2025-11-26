#include "pch.h"
#include "ParticleModuleRotation.h"
#include "ParticleEmitterInstances.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"

void UParticleModuleRotation::Spawn(const FSpawnContext& SpawnContext)
{
	FVector StartRotationValue = StartRotation.GetValue(SpawnContext.SpawnTime, FMath::FRand());
	if (!SpawnContext.Owner->CurrentLODLevel->TypeDataModule)
		StartRotationValue.X = StartRotationValue.Y = 0;

	SpawnContext.ParticleBase->Rotation = FQuat::MakeFromEulerZYX(StartRotationValue);
	SpawnContext.ParticleBase->RotationRate = FQuat::MakeFromEulerZYX(StartRotationValue);
}

void UParticleModuleRotation::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON StartRotationJson = InOutHandle["StartRotation"];
		StartRotation.Serialize(true, StartRotationJson);
	}
	else
	{
		JSON StartRotationJson = JSON::Make(JSON::Class::Object);
		StartRotation.Serialize(false, StartRotationJson);
		InOutHandle["StartRotation"] = StartRotationJson;
	}
}
