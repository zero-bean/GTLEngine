#include "pch.h"
#include "ParticleModuleColor.h"
#include "ParticleEmitterInstances.h"

void UParticleModuleColor::Spawn(const FSpawnContext& SpawnContext)
{
	// 이미터의 정규화된 시간(0.0~1.0)을 사용하여 커브 샘플링
	float NormalizedTime = SpawnContext.GetNormalizedEmitterTime();
	FVector StartColorVector = StartColor.GetValue(NormalizedTime, FMath::FRand());
	SpawnContext.ParticleBase->Color = FLinearColor(StartColorVector);
}

void UParticleModuleColor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON StartColorJson = InOutHandle["StartColor"];
		StartColor.Serialize(true, StartColorJson);
	}
	else
	{
		JSON StartColorJson = JSON::Make(JSON::Class::Object);
		StartColor.Serialize(false, StartColorJson);
		InOutHandle["StartColor"] = StartColorJson;
	}
}
