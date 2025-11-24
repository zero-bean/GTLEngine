#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleColor.generated.h"

UCLASS()
class UParticleModuleColor : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:

	FRawDistributionVector StartColor{ EDistributionMode::DOP_Uniform, FVector(), FVector(), FVector(1,1,1)};

	UParticleModuleColor() = default;

	void Spawn(const FSpawnContext& SpawnContext) override;
};