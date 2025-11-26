#include "pch.h"
#include "ParticleModuleSize.h"
#include "ParticleEmitterInstances.h"

UParticleModuleSize::UParticleModuleSize()
{
	bSpawn = true;
	bUpdate = false;
}

void UParticleModuleSize::Spawn(const FSpawnContext& SpawnContext)
{
	FVector StartSizeVector{ 1,1,1 };
	if (bEnabled)
	{
		// 이미터의 정규화된 시간(0.0~1.0)을 사용하여 커브 샘플링
		float NormalizedTime = SpawnContext.GetNormalizedEmitterTime();
		StartSizeVector = StartSize.GetValue(NormalizedTime, FMath::FRand());
	}

	SpawnContext.ParticleBase->Size = StartSizeVector;

}

void UParticleModuleSize::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON StartSizeJson = InOutHandle["StartSize"];
		StartSize.Serialize(true, StartSizeJson);
	}
	else
	{
		JSON StartSizeJson = JSON::Make(JSON::Class::Object);
		StartSize.Serialize(false, StartSizeJson);
		InOutHandle["StartSize"] = StartSizeJson;
	}
}
