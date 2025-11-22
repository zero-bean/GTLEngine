#pragma once
#include "ParticleModule.h"
#include "Distribution.h"

class UParticleModuleColor : public UParticleModule
{
public:

	FRawDistributionVector StartColor{ EDistributionMode::DOP_Uniform, FVector(), FVector(), FVector(1,1,1)};
	void Spawn(const FSpawnContext& SpawnContext) override;
};