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
#include "ParticleModuleTypeDataBeam.h"
#include "ParticleModuleTypeDataMesh.h"

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
		Required->Material = UResourceManager::GetInstance().Load<UMaterial>("Shaders/Particles/SpriteParticle.hlsl");
		Required->EmitterDuration = 2.0f;
		Required->EmitterLoops = 0;
		
		UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
		SpawnModule->SpawnRate.Operation = EDistributionMode::DOP_Constant;
		SpawnModule->SpawnRate.Constant = 0.0f;

		UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
		ColorModule->StartColor.Operation = EDistributionMode::DOP_Uniform;
		ColorModule->StartColor.Min = FVector(0.0f,0.0f,0.0f);
		ColorModule->StartColor.Max = FVector(1.0f,1.0f,1.0f);

		UParticleModuleTypeDataBeam* BeamType = NewObject<UParticleModuleTypeDataBeam>();
		BeamType->BeamMethod = EBeamMethod::EBM_Target;
		BeamType->BeamTaperMethod = EBeamTaperMethod::EBTM_None;
		BeamType->TextureTile = 1;
		BeamType->TextureTileDistance = 0.0f;
		BeamType->Sheets = 1;
		BeamType->MaxBeamCount = 1;
		BeamType->Speed = 0.0f;
		BeamType->BaseWidth = 5.0f;
		BeamType->InterpolationPoints = 50;
		BeamType->bAlwaysOn = 1;
		BeamType->UpVectorStepSize = 0;
		BeamType->Distance = FRawDistributionFloat(EDistributionMode::DOP_Constant, 0.0f, 0.0f, 0.0f);
		BeamType->TaperFactor = FRawDistributionFloat(EDistributionMode::DOP_Uniform, 1.0f, 1.0f, 1.0f);
		BeamType->TaperScale = FRawDistributionFloat(EDistributionMode::DOP_Uniform, 1.0f, 1.0f, 1.0f);
		BeamType->SourcePosition = FVector(0.0f, 0.0f, 0.0f);
		BeamType->TargetPosition = FVector(500.0f, 0.0f, 0.0f);
		BeamType->SourceTangent = FVector(200.0f, 300.0f, 200.0f);
		BeamType->TargetTangent = FVector(200.0f, -300.0f, -200.0f);

		LODLevel->TypeDataModule = BeamType;
		
		LODLevel->Modules.Add(ColorModule);		
		LODLevel->SpawnModules.Add(ColorModule);		
		
		LODLevel->SpawnModule = SpawnModule;
		
		LODLevel->RequiredModule = Required;

		LODLevel->Modules.Add(ColorModule);
		LODLevel->SpawnModules.Add(ColorModule);

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
