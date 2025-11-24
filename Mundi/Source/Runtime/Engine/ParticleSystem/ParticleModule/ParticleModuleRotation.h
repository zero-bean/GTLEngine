#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleRotation.generated.h"

UCLASS()
class UParticleModuleRotation : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:

	FRawDistributionFloat StartRotation{ EDistributionMode::DOP_Uniform, 45.0f, 0.0f, 90.0f };

	UParticleModuleRotation() = default;

	void Spawn(const FSpawnContext& SpawnContext) override;
};
