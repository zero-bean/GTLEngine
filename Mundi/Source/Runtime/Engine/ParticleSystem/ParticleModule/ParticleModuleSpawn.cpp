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
