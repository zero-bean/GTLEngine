#include "pch.h"
#include "ParticleLODLevel.h"

UParticleLODLevel::UParticleLODLevel()
{
}

UParticleLODLevel::~UParticleLODLevel()
{
	if (RequiredModule)
	{
		DeleteObject(RequiredModule);
		RequiredModule = nullptr;
	}
	if (SpawnModule)
	{
		DeleteObject(SpawnModule);
		SpawnModule = nullptr;
	}
	if (TypeDataModule)
	{
		DeleteObject(TypeDataModule);
		TypeDataModule = nullptr;
	}

	for (int32 Index = 0; Index < Modules.Num(); Index++)
	{
		DeleteObject(Modules[Index]);
		Modules[Index] = nullptr;
	}
	Modules.Empty();
	SpawnModules.Empty();
	UpdateModules.Empty();
}
