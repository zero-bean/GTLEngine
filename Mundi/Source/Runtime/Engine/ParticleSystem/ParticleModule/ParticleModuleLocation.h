#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
class UParticleModuleLocation : public UParticleModule
{
public:

	FRawDistributionVector SpawnLocation{ EDistributionMode::DOP_Uniform, FVector::Zero(), FVector::Zero(), FVector(1,1,1) };
	void Spawn(const FSpawnContext& SpawnContext) override;
};