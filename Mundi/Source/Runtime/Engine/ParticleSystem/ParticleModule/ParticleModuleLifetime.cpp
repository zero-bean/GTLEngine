#include "pch.h"
#include "ParticleModuleLifetime.h"
#include "Distribution.h"
#include "ParticleEmitterInstances.h"

UParticleModuleLifetime::UParticleModuleLifetime()
{
	bSpawn = true;
	bUpdate = false;
}

// UParticleModuleLifetime은 업데이트가 필요 없음
void UParticleModuleLifetime::Spawn(const FSpawnContext& SpawnContext)
{
	if (bEnabled)
	{
		// 이미터의 정규화된 시간(0.0~1.0)을 사용하여 커브 샘플링
		float NormalizedTime = SpawnContext.GetNormalizedEmitterTime();
		float LifeTimeValue = LifeTime.GetValue(NormalizedTime, FMath::FRand());
		SpawnContext.ParticleBase->Lifetime = LifeTimeValue;
		SpawnContext.ParticleBase->OneOverMaxLiftTime = 1.0f / LifeTimeValue;
		SpawnContext.ParticleBase->RelativeTime = 0.0f;
	}
	else
	{
		SpawnContext.ParticleBase->Lifetime = 0.0f;
		SpawnContext.ParticleBase->OneOverMaxLiftTime = 1.0f / 0.001f;
		SpawnContext.ParticleBase->RelativeTime = 1.0f;
	}
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
