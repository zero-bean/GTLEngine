#include "pch.h"
#include "ParticleModuleRotation.h"
#include "ParticleEmitterInstances.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"

void UParticleModuleRotation::Spawn(const FSpawnContext& SpawnContext)
{
	// 이미터의 정규화된 시간(0.0~1.0)을 사용하여 커브 샘플링
	float NormalizedTime = SpawnContext.GetNormalizedEmitterTime();
	FVector StartRotationValue = StartRotation.GetValue(NormalizedTime, FMath::FRand());
	if (!SpawnContext.Owner->CurrentLODLevel->TypeDataModule)
		StartRotationValue.X = StartRotationValue.Y = 0;

	SpawnContext.ParticleBase->Rotation = FQuat::MakeFromEulerZYX(StartRotationValue);
	SpawnContext.ParticleBase->RotationRate = FQuat::MakeFromEulerZYX(StartRotationValue);
}

void UParticleModuleRotation::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON StartRotationJson = InOutHandle["StartRotation"];
		StartRotation.Serialize(true, StartRotationJson);
	}
	else
	{
		JSON StartRotationJson = JSON::Make(JSON::Class::Object);
		StartRotation.Serialize(false, StartRotationJson);
		InOutHandle["StartRotation"] = StartRotationJson;
	}
}
