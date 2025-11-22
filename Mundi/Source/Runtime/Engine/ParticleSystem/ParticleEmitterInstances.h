#pragma once

class UParticleEmitter;
class UParticleSystemComponent;
class UParticleLODLevel;
class UParticleModule;
struct FBaseParticle;
struct FDynamicEmitterDataBase;


struct FParticleEmitterInstance
{
public:

	uint32 GetModuleOffset(UParticleModule* InModule);

	UParticleEmitter* SpriteTemplate = nullptr;

	UParticleSystemComponent* OwnerComponent = nullptr;

	UParticleLODLevel* CurrentLODLevel = nullptr;

	int32 CurrentLODLevelIndex = 0;

	uint8* ParticleData = nullptr;

	uint16* ParticleIndices = nullptr;

	// 현재 인스턴스 생성 이후 얼마나 지났는지 등 인스턴스 상태 저장.(모듈들이 이 인스턴스 데이터를 파티클처럼 쪼개서 각각 본인의 상태를 저장함)
	uint8* InstanceData = nullptr;

	int32 InstancePayloadSize = 0;

	int32 PayloadOffset = 0;

	// 파티클 하나의 바이트 단위 사이즈
	int32 ParticleSize = 0;

	// ParticleData Array에서 파티클 사이 Stride
	uint32 ParticleStride = 0;

	// 현재 활성화된 파티클 개수
	uint32 ActiveParticles = 0;

	// 파티클 ID로 씀. GUObjectArray의 InternalIndex같은 느낌.
	uint32 ParticleCounter = 0;

	// ActiveParticles의 최대 개수
	uint32 MaxActiveParticles = 0;

	UMaterialInterface* CurrentMaterial = nullptr;

	float EmitterTime;

	FParticleEmitterInstance(UParticleSystemComponent* InComponent);

	virtual FDynamicEmitterDataBase* GetDynamicData(bool bSelected) {
		return nullptr;
	}

	void InitParticles();

	void SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity);

	void PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity);

	void PostSpawn(FBaseParticle* InParticle, float InterpolationPercentage, float SpawnTime);
};

struct FParticleSpriteEmitterInstance : FParticleEmitterInstance
{
public:
	// ParticleSystemComponent 헤더가 방대해질 가능성이 높음, 인터페이스를 만들어야 함.
	FParticleSpriteEmitterInstance(UParticleSystemComponent* InComponent);
	FDynamicEmitterDataBase* GetDynamicData(bool bSelected) override;
};