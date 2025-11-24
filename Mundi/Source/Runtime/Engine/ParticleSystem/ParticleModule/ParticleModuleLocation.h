#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleLocation.generated.h"

UCLASS()
class UParticleModuleLocation : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:
	

	FRawDistributionVector SpawnLocation{ EDistributionMode::DOP_Uniform, FVector::Zero(), FVector::Zero(), FVector(1,1,1) };

	UParticleModuleLocation() = default;

	void Spawn(const FSpawnContext& SpawnContext) override;
};