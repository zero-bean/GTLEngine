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
#include "JsonSerializer.h"
#include "ObjectFactory.h"
#include "Source/Runtime/Core/Misc/PathUtils.h"

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
		Required->Material = UResourceManager::GetInstance().Load<UMaterial>("Shaders/UI/Billboard.hlsl");
		Required->EmitterDuration = 2.0f;
		Required->EmitterLoops = 0;

		UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
		ColorModule->StartColor.Operation = EDistributionMode::DOP_Uniform;
		ColorModule->StartColor.Min = FVector(0.0f,0.0f,0.0f);
		ColorModule->StartColor.Max = FVector(1.0f,1.0f,1.0f);

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
		VelocityModule->StartVelocity.Min = FVector(-10, -10, -10);
		VelocityModule->StartVelocity.Max = FVector(10, 10, 10);

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

void UParticleSystem::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	auto ResetEmitters = [this]()
	{
		for (UParticleEmitter* Emitter : Emitters)
		{
			if (Emitter)
			{
				DeleteObject(Emitter);
			}
		}
		Emitters.Empty();
	};

	if (bInIsLoading)
	{
		ResetEmitters();

		JSON EmittersJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "Emitters", EmittersJson, nullptr, false))
		{
			for (uint32 Index = 0; Index < static_cast<uint32>(EmittersJson.size()); ++Index)
			{
				JSON EmitterJson = EmittersJson.at(Index);

				FString TypeString;
				FJsonSerializer::ReadString(EmitterJson, "Type", TypeString, "UParticleEmitter", false);

				UClass* EmitterClass = !TypeString.empty() ? UClass::FindClass(TypeString) : nullptr;
				if (!EmitterClass || !EmitterClass->IsChildOf(UParticleEmitter::StaticClass()))
				{
					EmitterClass = UParticleEmitter::StaticClass();
				}

				if (UParticleEmitter* NewEmitter = Cast<UParticleEmitter>(ObjectFactory::NewObject(EmitterClass)))
				{
					NewEmitter->Serialize(true, EmitterJson);
					NewEmitter->CacheEmitterModuleInfo();
					Emitters.Add(NewEmitter);
				}
			}
		}
	}
	else
	{
		InOutHandle["Version"] = 1;

		JSON EmittersJson = JSON::Make(JSON::Class::Array);
		for (UParticleEmitter* Emitter : Emitters)
		{
			if (!Emitter)
			{
				continue;
			}

			JSON EmitterJson = JSON::Make(JSON::Class::Object);
			EmitterJson["Type"] = Emitter->GetClass()->Name;
			Emitter->Serialize(false, EmitterJson);
			EmittersJson.append(EmitterJson);
		}
		InOutHandle["Emitters"] = EmittersJson;
	}
}

bool UParticleSystem::Load(const FString& InFilePath, ID3D11Device* /*InDevice*/)
{
	if (InFilePath.empty())
	{
		return false;
	}

	FWideString FilePath = UTF8ToWide(InFilePath);
	JSON LoadedJson;
	if (!FJsonSerializer::LoadJsonFromFile(LoadedJson, FilePath))
	{
		UE_LOG("UParticleSystem::Load - Failed to load %s", InFilePath.c_str());
		return false;
	}

	Serialize(true, LoadedJson);
	return true;
}

bool UParticleSystem::Save(const FString& InFilePath) const
{
	if (InFilePath.empty())
	{
		return false;
	}

	JSON RootJson = JSON::Make(JSON::Class::Object);
	UParticleSystem* MutableThis = const_cast<UParticleSystem*>(this);
	MutableThis->Serialize(false, RootJson);

	FWideString FilePath = UTF8ToWide(InFilePath);
	return FJsonSerializer::SaveJsonToFile(RootJson, FilePath);
}
