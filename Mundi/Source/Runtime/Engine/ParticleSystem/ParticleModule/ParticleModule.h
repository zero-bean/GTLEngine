#pragma once
#include "Object.h"
#include "ParticleHelper.h"
#include "UParticleModule.generated.h"

struct FParticleEmitterInstance;
class UParticleModuleTypeDataBase;

struct FSpawnContext
{
	FParticleEmitterInstance* Owner;
	int32 PayloadOffset;
	float SpawnTime;
	FBaseParticle* ParticleBase;

	FSpawnContext(FParticleEmitterInstance* InOwner, int32 InPayloadOffset, float InSpawnTime, FBaseParticle* InParticleBase)
		:Owner(InOwner), PayloadOffset(InPayloadOffset), SpawnTime(InSpawnTime), ParticleBase(InParticleBase) {
	}

	// 이미터의 정규화된 시간(0.0~1.0) 반환 - 커브 샘플링용
	float GetNormalizedEmitterTime() const;
};

struct FUpdateContext
{
	FParticleEmitterInstance* Owner;
	int32 PayloadOffset;
	float DeltaTime;

	FUpdateContext(FParticleEmitterInstance* InOwner, int32 InPayloadOffset, float InDeltaTime)
		:Owner(InOwner), PayloadOffset(InPayloadOffset), DeltaTime(InDeltaTime) {
	}
};
UCLASS(DisplayName = "파티클 모듈", Description = "파티클 모듈")
class UParticleModule : public UObject
{
	GENERATED_REFLECTION_BODY()
public:

	UParticleModule() = default;
	bool bEnabled = true;
	bool bUpdate = false;
	bool bSpawn = true;

	virtual void Spawn(const FSpawnContext& SpawnContext);
	virtual void Update(const FUpdateContext& UpdateContext);
	virtual uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData);
	virtual uint32 RequiredBytesPerInstance();
	virtual void PrepPerInstanceBlock(FParticleEmitterInstance* EmitterInstance, void* InstanceData);

	// JSON 직렬화
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};