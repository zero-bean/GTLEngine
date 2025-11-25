#include "pch.h"
#include "ParticleModuleLifetime.h"
#include "Distribution.h"
#include "ParticleEmitterInstances.h"

// UParticleModuleLifetime은 업데이트가 필요 없음
void UParticleModuleLifetime::Spawn(const FSpawnContext& SpawnContext)
{
	float LifeTimeValue= LifeTime.GetValue(SpawnContext.Owner->EmitterTime, FMath::FRand());
	SpawnContext.ParticleBase->Lifetime = LifeTimeValue;
	SpawnContext.ParticleBase->OneOverMaxLiftTime = 1.0f / LifeTimeValue;
	SpawnContext.ParticleBase->RelativeTime = 0.0f;
}

void UParticleModuleLifetime::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON LifeTimeJson = InOutHandle["LifeTime"];
		LifeTime.Serialize(true, LifeTimeJson);
	}
	else
	{
		JSON LifeTimeJson = JSON::Make(JSON::Class::Object);
		LifeTime.Serialize(false, LifeTimeJson);
		InOutHandle["LifeTime"] = LifeTimeJson;
	}
}
