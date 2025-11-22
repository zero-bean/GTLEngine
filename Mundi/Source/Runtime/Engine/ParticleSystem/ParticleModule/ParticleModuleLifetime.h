#pragma once
#include "ParticleModule.h"
#include "Distribution.h"

class UParticleModuleLifetime : public UParticleModule
{
public:
	FRawDistributionFloat LifeTime{ EDistributionMode::DOP_Uniform, 1.0f, 0.0f, 1.0f };
	void Spawn(const FSpawnContext& SpawnContext) override;
};