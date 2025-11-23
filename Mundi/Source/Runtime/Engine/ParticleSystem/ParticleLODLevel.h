#pragma once
#include "Object.h"

class UParticleModule;
class UParticleModuleTypeDataBase;
class UParticleModuleRequired;
class UParticleModuleSpawn;

class UParticleLODLevel : public UObject
{
public:

    int32 Level;
    bool bEnabled;

    // 파티클이 가장 많을 때 개수(최적화 전용)
    int32 PeakActiveParticles = 0;

    UParticleModuleRequired* RequiredModule;

    TArray<UParticleModule*> Modules;

    // 파티클 생성 시에 값을 초기화시켜주는 모듈 배열
    TArray<UParticleModule*> SpawnModules;

    // 파티클 생성 개수, 속도 등을 담당하는 모듈(위의 SpawnModules랑 혼동 주의!!)
    UParticleModuleSpawn* SpawnModule = nullptr;

    TArray<UParticleModule*> UpdateModules;

    UParticleModuleTypeDataBase* TypeDataModule;
};