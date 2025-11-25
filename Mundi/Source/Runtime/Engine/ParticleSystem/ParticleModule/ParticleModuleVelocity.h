#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleVelocity.generated.h"

UCLASS()
class UParticleModuleVelocity : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:
	FRawDistributionVector StartVelocity{ EDistributionMode::DOP_Uniform, FVector(), FVector(-1,-1,-1), FVector(1,1,1) };

	UParticleModuleVelocity() = default;

	void Spawn(const FSpawnContext& SpawnContext) override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
