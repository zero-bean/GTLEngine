#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleModuleRequired.h"
#include "ParticleModuleColor.h"
#include "ParticleModuleLifetime.h"
#include "ParticleModuleLocation.h"
#include "ParticleModuleSize.h"
#include "ParticleModuleSpawn.h"
#include "ParticleModuleVelocity.h"
#include "ParticleModuleRotation.h"

UParticleSystem* UParticleSystem::TestParticleSystem = nullptr;

UParticleSystem::UParticleSystem()
{
}
UParticleSystem::~UParticleSystem()
{
	for (int32 Index = 0; Index < Emitters.Num(); Index++)
	{
		DeleteObject(Emitters[Index]);
	}
	Emitters.Empty();
}

UParticleSystem* UParticleSystem::GetTestParticleSystem()
{
	if (!TestParticleSystem)
	{
		TestParticleSystem = NewObject<UParticleSystem>();

		UParticleEmitter* Emitter = NewObject<UParticleEmitter>();

		UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();

		Emitter->LODLevels.Add(LODLevel);
		TestParticleSystem->Emitters.Add(Emitter);

		LODLevel->Level = 0;
		LODLevel->bEnabled = true;

		UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();

		Required->Material = UResourceManager::GetInstance().GetDefaultMaterial();
		Required->EmitterDuration = 2.0f;
		Required->EmitterLoops = 0;

		UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
		ColorModule->StartColor.Operation = EDistributionMode::DOP_Constant;
		ColorModule->StartColor.Constant = FVector(1.0f,1.0f,1.0f);

		UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
		LifetimeModule->LifeTime.Operation = EDistributionMode::DOP_Uniform;
		LifetimeModule->LifeTime.Min = 1.0f;
		LifetimeModule->LifeTime.Max = 5.0f;

		UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
		LocationModule->SpawnLocation.Operation = EDistributionMode::DOP_Uniform;
		LocationModule->SpawnLocation.Min = FVector(-1.0f, -1.0f, -1.0f);
		LocationModule->SpawnLocation.Max = FVector(1.0f, 1.0f, 1.0f);

		UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
		SizeModule->StartSize.Operation = EDistributionMode::DOP_Uniform;
		SizeModule->StartSize.Min = FVector(1.0f, 1.0f, 1.0f);
		SizeModule->StartSize.Max = FVector(3.0f, 3.0f, 3.0f);

		UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
		SpawnModule->SpawnRate.Operation = EDistributionMode::DOP_Constant;
		SpawnModule->SpawnRate.Constant = 30.0f;

		UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
		VelocityModule->StartVelocity.Operation = EDistributionMode::DOP_Uniform;
		VelocityModule->StartVelocity.Min = FVector(-1, -1, -1);
		VelocityModule->StartVelocity.Max = FVector(1, 1, 1);

		UParticleModuleRotation* RotationModule = NewObject<UParticleModuleRotation>();
		RotationModule->StartRotation.Operation = EDistributionMode::DOP_Uniform;
		RotationModule->StartRotation.Min = 0.0f;
		RotationModule->StartRotation.Max = 90.0f;
		

		LODLevel->SpawnModule = SpawnModule;
		LODLevel->RequiredModule = Required;
		// SpriteInstance사용
		LODLevel->TypeDataModule = nullptr;
		LODLevel->Modules.Add(ColorModule); LODLevel->Modules.Add(LifetimeModule); LODLevel->Modules.Add(LocationModule); 
		LODLevel->Modules.Add(SizeModule); LODLevel->Modules.Add(VelocityModule); LODLevel->Modules.Add(RotationModule);

		LODLevel->SpawnModules.Add(ColorModule); LODLevel->SpawnModules.Add(LifetimeModule); LODLevel->SpawnModules.Add(LocationModule);
		LODLevel->SpawnModules.Add(SizeModule); LODLevel->SpawnModules.Add(VelocityModule); LODLevel->SpawnModules.Add(RotationModule);

		Emitter->CacheEmitterModuleInfo();
		// Update모듈 없음.
		
	}


	return TestParticleSystem;
}

void UParticleSystem::ReleaseTestParticleSystem()
{
	if (TestParticleSystem)
	{
		DeleteObject(TestParticleSystem);
		TestParticleSystem = nullptr;
	}
}
