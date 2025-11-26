#include "pch.h"
#include "ParticleModuleVelocity.h"
#include "ParticleEmitterInstances.h"

UParticleModuleVelocity::UParticleModuleVelocity()
{
	bSpawn = true;
	bUpdate = false;
}

void UParticleModuleVelocity::Spawn(const FSpawnContext& SpawnContext)
{
	// 이미터의 정규화된 시간(0.0~1.0)을 사용하여 커브 샘플링
	float NormalizedTime = SpawnContext.GetNormalizedEmitterTime();
	FVector StartVelocityVector = StartVelocity.GetValue(NormalizedTime, FMath::FRand());
	// 이미터에 대한 상대속도
	SpawnContext.ParticleBase->BaseVelocity += StartVelocityVector;
	SpawnContext.ParticleBase->Velocity += StartVelocityVector;
}

void UParticleModuleVelocity::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON StartVelocityJson = InOutHandle["StartVelocity"];
		StartVelocity.Serialize(true, StartVelocityJson);
	}
	else
	{
		JSON StartVelocityJson = JSON::Make(JSON::Class::Object);
		StartVelocity.Serialize(false, StartVelocityJson);
		InOutHandle["StartVelocity"] = StartVelocityJson;
	}
}
