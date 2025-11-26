#include "pch.h"
#include "ParticleHelper.h"
#include "ParticleModuleRotationRate.h"
#include "ParticleEmitterInstances.h"

UParticleModuleRotationRate::UParticleModuleRotationRate()
{
	bUpdate = true;
	bSpawn = false;
}

void UParticleModuleRotationRate::Update(const FUpdateContext& UpdateContext)
{
	
	DECLARE_PARTICLE_DATA_FROM_OWNER(UpdateContext)
	BEGIN_UPDATE_LOOP
		FVector RotationRateValue = RotationRate.GetValue(Particle.RelativeTime);
		Particle.RotationRate = FQuat::MakeFromEulerZYX(RotationRateValue);
	END_UPDATE_LOOP
}

void UParticleModuleRotationRate::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{

	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON RotationRateJson = InOutHandle["RotationRate"];
		RotationRate.Serialize(true, RotationRateJson);
	}
	else
	{
		JSON RotationRateJson = JSON::Make(JSON::Class::Object);
		RotationRate.Serialize(false, RotationRateJson);
		InOutHandle["RotationRate"] = RotationRateJson;
	}
}
