#pragma once
#include "ParticleModule.h"
#include "Distribution.h"

class UParticleModuleSize : UParticleModule
{
public:
	FRawDistributionVector StartSize{ EDistributionMode::DOP_Uniform, FVector::Zero(), FVector(2,2,2), FVector(5,5,5) };
	void Spawn(const FSpawnContext& SpawnContext) override;
};
