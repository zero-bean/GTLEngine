#pragma once
#include "ParticleModule.h"
#include "Distribution.h"

class UParticleModuleVelocity : public UParticleModule
{
public:
	FRawDistributionVector StartVelocity{ EDistributionMode::DOP_Uniform, FVector(), FVector(-1,-1,-1), FVector(1,1,1) };
	void Spawn(const FSpawnContext& SpawnContext) override;
};
