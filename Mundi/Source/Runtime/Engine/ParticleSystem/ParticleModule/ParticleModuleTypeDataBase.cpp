#include "pch.h"
#include "ParticleModuleTypeDataBase.h"

void UParticleModuleTypeDataBase::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
}

UParticleModuleTypeDataBase::UParticleModuleTypeDataBase()
{
	bSpawn = false;
	bUpdate = false;
}
