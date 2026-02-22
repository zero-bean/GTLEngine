#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleLifetime.generated.h"

UCLASS()
class UParticleModuleLifetime : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:
	

	FRawDistributionFloat LifeTime{ EDistributionMode::DOP_Uniform, 1.0f, 0.0f, 1.0f };

	UParticleModuleLifetime();

	void Spawn(const FSpawnContext& SpawnContext) override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};