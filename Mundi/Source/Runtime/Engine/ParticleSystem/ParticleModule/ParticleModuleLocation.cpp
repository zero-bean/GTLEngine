#include "pch.h"
#include "ParticleModuleLocation.h"
#include "ParticleEmitterInstances.h"

// UParticleModuleLocation은 업데이트가 필요 없음, 생성될 좌표만 결정
void UParticleModuleLocation::Spawn(const FSpawnContext& SpawnContext)
{
	FVector SpawnLocationVector = SpawnLocation.GetValue(SpawnContext.Owner->EmitterTime);
	// 이미터의 현재 위치의 상대좌표라서 더함.
	SpawnContext.ParticleBase->Location += SpawnLocationVector;
}

void UParticleModuleLocation::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON SpawnLocationJson = InOutHandle["SpawnLocation"];
		SpawnLocation.Serialize(true, SpawnLocationJson);
	}
	else
	{
		JSON SpawnLocationJson = JSON::Make(JSON::Class::Object);
		SpawnLocation.Serialize(false, SpawnLocationJson);
		InOutHandle["SpawnLocation"] = SpawnLocationJson;
	}
}
