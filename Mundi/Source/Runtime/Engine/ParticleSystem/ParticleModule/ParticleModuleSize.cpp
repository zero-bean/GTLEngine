#include "pch.h"
#include "ParticleModuleSize.h"
#include "ParticleEmitterInstances.h"

void UParticleModuleSize::Spawn(const FSpawnContext& SpawnContext)
{
	FVector StartSizeVector = StartSize.GetValue(SpawnContext.Owner->EmitterTime, FMath::FRand());

	SpawnContext.ParticleBase->Size = StartSizeVector;

}

void UParticleModuleSize::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON StartSizeJson = InOutHandle["StartSize"];
		StartSize.Serialize(true, StartSizeJson);
	}
	else
	{
		JSON StartSizeJson = JSON::Make(JSON::Class::Object);
		StartSize.Serialize(false, StartSizeJson);
		InOutHandle["StartSize"] = StartSizeJson;
	}
}
