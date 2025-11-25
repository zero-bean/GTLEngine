#include "pch.h"
#include "ParticleModuleSpawn.h"

uint32 UParticleModuleSpawn::RequiredBytesPerInstance()
{
    return sizeof(FParticleSpawnInstanceData);
}

int32 UParticleModuleSpawn::GetSpawnCount(float DeltaTime, float& SpawnFraction, float EmitterTime)
{
    float SpawnRateValue = SpawnRate.GetValue(DeltaTime);
    float SpawnCountFloat = (SpawnRateValue * DeltaTime) + SpawnFraction;

    int32 SpawnCount = FMath::FloorToInt(SpawnCountFloat);

    SpawnFraction = SpawnCountFloat - SpawnCount;

    return SpawnCount;
}

void UParticleModuleSpawn::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON SpawnRateJson = InOutHandle["SpawnRate"];
		SpawnRate.Serialize(true, SpawnRateJson);
	}
	else
	{
		JSON SpawnRateJson = JSON::Make(JSON::Class::Object);
		SpawnRate.Serialize(false, SpawnRateJson);
		InOutHandle["SpawnRate"] = SpawnRateJson;
	}
}
