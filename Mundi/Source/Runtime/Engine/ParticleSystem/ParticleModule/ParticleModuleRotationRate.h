#pragma once
#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleRotationRate.generated.h"

UCLASS()
class UParticleModuleRotationRate : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:

	FRawDistributionVector RotationRate{ EDistributionMode::DOP_Constant, 0.0f, 0.0f, 90.0f };

	UParticleModuleRotationRate();

	void Update(const FUpdateContext& UpdateContext) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
