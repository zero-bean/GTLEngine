#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleSize.generated.h"

UCLASS()
class UParticleModuleSize : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:

	FRawDistributionVector StartSize{ EDistributionMode::DOP_Uniform, FVector::Zero(), FVector(2,2,2), FVector(5,5,5) };

	UParticleModuleSize() = default;

	void Spawn(const FSpawnContext& SpawnContext) override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
