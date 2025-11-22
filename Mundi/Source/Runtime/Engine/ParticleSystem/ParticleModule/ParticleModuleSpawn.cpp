#include "pch.h"
#include "ParticleModuleSpawn.h"

uint32 UParticleModuleSpawn::RequiredBytesPerInstance()
{
    return sizeof(FParticleSpawnInstanceData);
}
