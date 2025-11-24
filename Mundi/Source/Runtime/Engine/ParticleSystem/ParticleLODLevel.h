#pragma once
#include "Object.h"
#include "UParticleLODLevel.generated.h"

class UParticleModule;
class UParticleModuleTypeDataBase;
class UParticleModuleRequired;
class UParticleModuleSpawn;

UCLASS()
class UParticleLODLevel : public UObject
{
    GENERATED_REFLECTION_BODY()
public:

    UParticleLODLevel();
    ~UParticleLODLevel();

    int32 Level = 0;
    bool bEnabled = true;

    // 파티클이 가장 많을 때 개수(최적화 전용)
    int32 PeakActiveParticles = 0;

    UParticleModuleRequired* RequiredModule = nullptr;

    // 파티클 생성 개수, 속도 등을 담당하는 모듈(SpawnModules랑 혼동 주의!!)
    UParticleModuleSpawn* SpawnModule = nullptr;

    UParticleModuleTypeDataBase* TypeDataModule = nullptr;

    TArray<UParticleModule*> Modules;

    // 파티클 생성 시에 값을 초기화시켜주는 모듈 배열
    TArray<UParticleModule*> SpawnModules;

    TArray<UParticleModule*> UpdateModules;
};