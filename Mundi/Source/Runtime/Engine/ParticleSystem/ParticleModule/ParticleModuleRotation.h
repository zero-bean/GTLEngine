#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleRotation.generated.h"

UCLASS()
class UParticleModuleRotation : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:

	FRawDistributionVector StartRotation{ EDistributionMode::DOP_Uniform, 45.0f, 0.0f, 90.0f };

	UParticleModuleRotation();

	void Spawn(const FSpawnContext& SpawnContext) override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
