#include "pch.h"
#include "ParticleModuleColor.h"
#include "ParticleEmitterInstances.h"

void UParticleModuleColor::Spawn(const FSpawnContext& SpawnContext)
{
	FVector StartColorVector = StartColor.GetValue(0.0f, FMath::FRand());
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
