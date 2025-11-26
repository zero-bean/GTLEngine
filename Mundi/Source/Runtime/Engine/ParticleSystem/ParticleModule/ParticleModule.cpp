#include "pch.h"
#include "ParticleModule.h"
#include "ParticleEmitterInstances.h"

float FSpawnContext::GetNormalizedEmitterTime() const
{
	if (Owner)
	{
		return Owner->GetNormalizedEmitterTime();
	}
	return 0.0f;
}

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

void UParticleModule::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		bEnabled = InOutHandle["bEnabled"].ToBool();
	}
	else
	{
		InOutHandle["bEnabled"] = bEnabled;
	}
}
