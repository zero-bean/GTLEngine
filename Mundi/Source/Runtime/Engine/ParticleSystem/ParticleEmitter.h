#pragma once
#include "Object.h"
#include "UParticleEmitter.generated.h"

class UParticleLODLevel;
class UParticleModule;
class UParticleSystemComponent;
struct FParticleEmitterInstance;

UCLASS()
class UParticleEmitter : public UObject
{
    GENERATED_REFLECTION_BODY()
public:

    UParticleEmitter();
    ~UParticleEmitter();

    FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InOwnerComponent);

    TArray<UParticleLODLevel*> LODLevels;

    // 인스턴스 데이터가 필요한 모듈 리스트
    // 인스턴스 초기화할때 필요함
    TArray<UParticleModule*> ModulesNeedingInstanceData;

    // 메시 머리티얼 모듈을 위한 배열
    TArray<class UMaterialInterface*> MeshMaterials;

    // 파티클 1개당 바이트 수
    int32 ParticleSize = 0;

    // 이미터 인스턴스 1개당 필요한 InstanceData 바이트 수
    int32 ReqInstanceBytes = 0;

    // 최적화 용도, 게임 시작하자마자 할당될 파티클 카운트 100개가 최대임
    int32 InitialAllocationCount = 0;

    // 모듈에 대응하는 오프셋 반환하는 맵
    TMap<UParticleModule*, uint32> ModuleOffsetMap;
    
    TMap<UParticleModule*, uint32> ModuleInstanceOffsetMap;

    void CacheEmitterModuleInfo();
};