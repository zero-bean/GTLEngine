#include "pch.h"
#include "ParticleModule.h"

void UParticleModule::Spawn(const FSpawnContext& SpawnContext)
{
}

void UParticleModule::Update(const FUpdateContext& UpdateContext)
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

void UParticleModule::PrepPerInstanceBlock(FParticleEmitterInstance* EmitterInstance, void* InstanceData)
{
}
