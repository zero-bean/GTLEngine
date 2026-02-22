#include "pch.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleModuleRequired.h"
#include "ParticleModuleSpawn.h"
#include "ParticleModuleTypeDataBase.h"
#include "JsonSerializer.h"
#include "ObjectFactory.h"

UParticleLODLevel::UParticleLODLevel()
{
}

UParticleLODLevel::~UParticleLODLevel()
{
	if (RequiredModule)
	{
		DeleteObject(RequiredModule);
		RequiredModule = nullptr;
	}
	if (SpawnModule)
	{
		DeleteObject(SpawnModule);
		SpawnModule = nullptr;
	}
	if (TypeDataModule)
	{
		DeleteObject(TypeDataModule);
		TypeDataModule = nullptr;
	}

	for (int32 Index = 0; Index < Modules.Num(); Index++)
	{
		DeleteObject(Modules[Index]);
		Modules[Index] = nullptr;
	}
	Modules.Empty();
	SpawnModules.Empty();
	UpdateModules.Empty();
}

static UParticleModule* LoadModuleFromJson(const JSON& ModuleJson, UClass* DefaultClass)
{
	if (ModuleJson.IsNull())
	{
		return nullptr;
	}

	FString TypeString;
	if (!FJsonSerializer::ReadString(ModuleJson, "Type", TypeString, 
		DefaultClass ? FString(DefaultClass->Name) : FString(), false))
	{
		TypeString = DefaultClass ? FString(DefaultClass->Name) : FString();
	}

	UClass* ModuleClass = nullptr;
	if (!TypeString.empty())
	{
		ModuleClass = UClass::FindClass(TypeString);
	}

	if (!ModuleClass)
	{
		ModuleClass = DefaultClass;
	}

	if (!ModuleClass || !ModuleClass->IsChildOf(UParticleModule::StaticClass()))
	{
		return nullptr;
	}

	if (UParticleModule* NewModule = Cast<UParticleModule>(ObjectFactory::NewObject(ModuleClass)))
	{
		JSON ModuleCopy = ModuleJson;
		NewModule->Serialize(true, ModuleCopy);
		return NewModule;
	}

	return nullptr;
}

void UParticleLODLevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	auto ResetModuleArray = [this]()
	{
		for (UParticleModule* Module : Modules)
		{
			if (Module)
			{
				DeleteObject(Module);
			}
		}
		Modules.Empty();
		SpawnModules.Empty();
		UpdateModules.Empty();
	};

	if (bInIsLoading)
	{
		if (RequiredModule)
		{
			DeleteObject(RequiredModule);
			RequiredModule = nullptr;
		}
		if (SpawnModule)
		{
			DeleteObject(SpawnModule);
			SpawnModule = nullptr;
		}
		if (TypeDataModule)
		{
			DeleteObject(TypeDataModule);
			TypeDataModule = nullptr;
		}

		ResetModuleArray();

		FJsonSerializer::ReadInt32(InOutHandle, "Level", Level, Level, false);
		FJsonSerializer::ReadBool(InOutHandle, "bEnabled", bEnabled, bEnabled, false);
		FJsonSerializer::ReadInt32(InOutHandle, "PeakActiveParticles", PeakActiveParticles, PeakActiveParticles, false);

		JSON RequiredModuleJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "RequiredModule", RequiredModuleJson, nullptr, false))
		{
			RequiredModule = Cast<UParticleModuleRequired>(LoadModuleFromJson(RequiredModuleJson, UParticleModuleRequired::StaticClass()));
		}

		JSON SpawnModuleJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "SpawnModule", SpawnModuleJson, nullptr, false))
		{
			SpawnModule = Cast<UParticleModuleSpawn>(LoadModuleFromJson(SpawnModuleJson, UParticleModuleSpawn::StaticClass()));
		}

		JSON TypeDataJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "TypeDataModule", TypeDataJson, nullptr, false))
		{
			TypeDataModule = Cast<UParticleModuleTypeDataBase>(LoadModuleFromJson(TypeDataJson, UParticleModuleTypeDataBase::StaticClass()));
		}

		JSON ModulesJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "Modules", ModulesJson, nullptr, false))
		{
			for (uint32 Index = 0; Index < static_cast<uint32>(ModulesJson.size()); ++Index)
			{
				JSON ModuleJson = ModulesJson.at(Index);
				if (UParticleModule* NewModule = LoadModuleFromJson(ModuleJson, UParticleModule::StaticClass()))
				{
					Modules.Add(NewModule);
				}
			}
		}

		auto ResolveModuleIndices = [this](const JSON& IndexArray, TArray<UParticleModule*>& OutModules)
		{
			for (uint32 ArrayIndex = 0; ArrayIndex < static_cast<uint32>(IndexArray.size()); ++ArrayIndex)
			{
				int32 ModuleIndex = IndexArray.at(ArrayIndex).ToInt();
				if (ModuleIndex >= 0 && ModuleIndex < Modules.Num())
				{
					OutModules.Add(Modules[ModuleIndex]);
				}
			}
		};

		JSON SpawnIndexJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "SpawnModuleIndices", SpawnIndexJson, nullptr, false))
		{
			ResolveModuleIndices(SpawnIndexJson, SpawnModules);
		}
		else
		{
			for (UParticleModule* Module : Modules)
			{
				SpawnModules.Add(Module);
			}
		}

		JSON UpdateIndexJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "UpdateModuleIndices", UpdateIndexJson, nullptr, false))
		{
			ResolveModuleIndices(UpdateIndexJson, UpdateModules);
		}
		else
		{
			UpdateModules.Empty();
		}
	}
	else
	{
		InOutHandle["Level"] = Level;
		InOutHandle["bEnabled"] = bEnabled;
		InOutHandle["PeakActiveParticles"] = PeakActiveParticles;

		if (RequiredModule)
		{
			JSON RequiredJson = JSON::Make(JSON::Class::Object);
			RequiredJson["Type"] = RequiredModule->GetClass()->Name;
			RequiredModule->Serialize(false, RequiredJson);
			InOutHandle["RequiredModule"] = RequiredJson;
		}

		if (SpawnModule)
		{
			JSON SpawnJson = JSON::Make(JSON::Class::Object);
			SpawnJson["Type"] = SpawnModule->GetClass()->Name;
			SpawnModule->Serialize(false, SpawnJson);
			InOutHandle["SpawnModule"] = SpawnJson;
		}

		if (TypeDataModule)
		{
			JSON TypeDataOutJson = JSON::Make(JSON::Class::Object);
			TypeDataOutJson["Type"] = TypeDataModule->GetClass()->Name;
			TypeDataModule->Serialize(false, TypeDataOutJson);
			InOutHandle["TypeDataModule"] = TypeDataOutJson;
		}

		JSON ModulesJson = JSON::Make(JSON::Class::Array);
		for (UParticleModule* Module : Modules)
		{
			if (!Module) { continue; }

			JSON ModuleJson = JSON::Make(JSON::Class::Object);
			ModuleJson["Type"] = Module->GetClass()->Name;
			Module->Serialize(false, ModuleJson);
			ModulesJson.append(ModuleJson);
		}
		InOutHandle["Modules"] = ModulesJson;

		auto WriteModuleIndices = [this](const TArray<UParticleModule*>& SourceModules)
		{
			JSON IndicesJson = JSON::Make(JSON::Class::Array);
			for (UParticleModule* Module : SourceModules)
			{
				if (!Module) { continue; }

				int32 ModuleIndex = -1;
				for (int32 Index = 0; Index < Modules.Num(); ++Index)
				{
					if (Modules[Index] == Module)
					{
						ModuleIndex = Index;
						break;
					}
				}

				if (ModuleIndex >= 0)
				{
					IndicesJson.append(ModuleIndex);
				}
			}

			return IndicesJson;
		};

		InOutHandle["SpawnModuleIndices"] = WriteModuleIndices(SpawnModules);
		InOutHandle["UpdateModuleIndices"] = WriteModuleIndices(UpdateModules);
	}
}
