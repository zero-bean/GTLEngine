#pragma once
#include "Object.h"

class UParticleModule;
class UParticleModuleTypeDataBase;
class UParticleModuleRequired;

class UParticleLODLevel : public UObject
{
public:

    int32 Level;
    bool bEnabled;

    UParticleModuleRequired* RequiredModule;
    TArray<UParticleModule*> Modules;
    TArray<UParticleModule*> SpawnModules;
    TArray<UParticleModule*> UpdateModules;
    UParticleModuleTypeDataBase* TypeDataModule;
};