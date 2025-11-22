#include "pch.h"
#include "ParticleModule.h"

void UParticleModule::Spawn(const FSpawnContext& SpawnContext)
{
}

uint32 UParticleModule::RequiredBytes(UParticleModuleTypeDataBase* TypeData)
{
	return 0;
}

uint32 UParticleModule::RequiredBytesPerInstance()
{
	return 0;
}
