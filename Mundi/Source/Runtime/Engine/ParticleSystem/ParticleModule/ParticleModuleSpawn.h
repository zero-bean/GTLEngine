#pragma once
#include "ParticleModule.h"

struct FParticleSpawnInstanceData
{
	float Accumulator;
	int32 BurstIndex;
};

class UParticleModuleSpawn : public UParticleModule
{

public:

	
	void Spawn(const FSpawnContext& SpawnContext) override;
	uint32 RequiredBytesPerInstance() override;
};