#include "pch.h"
#include "ParticleModuleLocation.h"
#include "ParticleEmitterInstances.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"
#include "ParticleSystemComponent.h"

UParticleModuleLocation::UParticleModuleLocation()
{
	bSpawn = true;
	bUpdate = false;
}

// UParticleModuleLocation은 업데이트가 필요 없음, 생성될 좌표만 결정
void UParticleModuleLocation::Spawn(const FSpawnContext& SpawnContext)
{
	FVector SpawnLocationVector{};
	if (bEnabled)
	{
		SpawnLocationVector = SpawnLocation.GetValue(SpawnContext.Owner->EmitterTime, FMath::FRand());
	}
	
	// 그냥 이미터에 대한 상대좌표라서 대입.
	if (SpawnContext.Owner->CurrentLODLevel->RequiredModule->bUseLocalSpace)
	{
		SpawnContext.ParticleBase->Location = SpawnLocationVector;
	}
	// 이미터의 현재 위치에 더한 월드 좌표라서 더함.
	else
	{
		FVector RotatedOffset = SpawnContext.Owner->OwnerComponent->GetWorldRotation().RotateVector(SpawnLocationVector);
		SpawnContext.ParticleBase->Location += RotatedOffset;
	}
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
