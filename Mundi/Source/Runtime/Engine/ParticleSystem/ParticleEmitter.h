#pragma once
#include "Object.h"

class UParticleLODLevel;
class UParticleModule;
class UParticleSystemComponent;
struct FParticleEmitterInstance;

class UParticleEmitter : public UObject
{
public:

    FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InOwnerComponent);

    TArray<UParticleLODLevel*> LODLevels;

    // 파티클 1개당 바이트 수
    int32 ParticleSize;

    // 이미터 인스턴스 1개당 필요한 InstanceData 바이트 수
    int32 ReqInstanceBytes;

    // 모듈에 대응하는 오프셋 반환하는 맵
    TMap<UParticleModule*, uint32> ModuleOffsetMap;
    
    TMap<UParticleModule*, uint32> ModuleInstanceOffsetMap;

    void CacheEmitterModuleInfo();
};