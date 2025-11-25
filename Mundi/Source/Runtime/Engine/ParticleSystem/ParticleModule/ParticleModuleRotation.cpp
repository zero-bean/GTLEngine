#include "pch.h"
#include "ParticleModuleRotation.h"
#include "ParticleModule.h"

void UParticleModuleRotation::Spawn(const FSpawnContext& SpawnContext)
{
	float StartRotationValue = StartRotation.GetValue(SpawnContext.SpawnTime, FMath::FRand());
	SpawnContext.ParticleBase->Rotation = StartRotationValue;
	SpawnContext.ParticleBase->RotationRate = StartRotationValue;
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
