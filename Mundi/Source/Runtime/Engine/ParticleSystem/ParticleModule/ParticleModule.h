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
};
UCLASS(DisplayName = "파티클 모듈", Description = "파티클 모듈")
class UParticleModule : public UObject
{
	GENERATED_REFLECTION_BODY()
public:

	UParticleModule() = default;
	bool bEnabled = true;

	virtual void Spawn(const FSpawnContext& SpawnContext);
	virtual uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData);
	virtual uint32 RequiredBytesPerInstance();
};