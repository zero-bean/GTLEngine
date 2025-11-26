#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleSpawn.generated.h"

struct FParticleSpawnInstanceData
{
	// 거리기반 누적, 1m당 몇개 생성..
	float Accumulator;
	int32 BurstIndex;
};

UCLASS()
class UParticleModuleSpawn : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:

	FRawDistributionFloat SpawnRate{ EDistributionMode::DOP_Constant, 10.0f, 0.0f, 0.0f };
	
	UParticleModuleSpawn();

	uint32 RequiredBytesPerInstance() override;

	int32 GetSpawnCount(float DeltaTime, float& SpawnFraction, float EmitterTime);

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};