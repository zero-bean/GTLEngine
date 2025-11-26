#include "pch.h"
#include "ParticleHelper.h"
#include "ParticleEmitter.h"
#include "ParticleEmitterInstances.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "JsonSerializer.h"
#include "ObjectFactory.h"
#include "ResourceManager.h"
#include "Material.h"
#include "ParticleModuleTypeDataBeam.h"
#include "ParticleModuleTypeDataMesh.h"

UParticleEmitter::UParticleEmitter()
{
}

UParticleEmitter::~UParticleEmitter()
{
	for (int32 Index = 0; Index < LODLevels.Num(); Index++)
	{
		DeleteObject(LODLevels[Index]);
	}
	LODLevels.Empty();
}

// 이미터가 본인에 대해 제일 잘 알기 때문에 이미터가 직접 인스턴스 생성하는게 효율적임
FParticleEmitterInstance* UParticleEmitter::CreateInstance(UParticleSystemComponent* InOwnerComponent)
{
	
	if (LODLevels.Num() == 0)
	{
		return nullptr;
	}

	FParticleEmitterInstance* NewInstance = nullptr;
	UParticleModuleTypeDataBase* TypeData = LODLevels[0]->TypeDataModule; 
	if (auto* BeamType = dynamic_cast<UParticleModuleTypeDataBeam*>(TypeData))
	{
		NewInstance = new FParticleBeamEmitterInstance(InOwnerComponent);
	}
	else if (auto* MeshType = dynamic_cast<UParticleModuleTypeDataMesh*>(TypeData))
	{
		NewInstance = new FParticleMeshEmitterInstance(InOwnerComponent);
	}
	else
	{
		NewInstance = new FParticleSpriteEmitterInstance(InOwnerComponent);
	}

	if (!NewInstance)
	{
		return nullptr;
	}
	// ParticleEmitter 설계도 인스턴스에 설정
	NewInstance->SpriteTemplate = this;

	return NewInstance;
}

// LOD 0레벨을 기준으로 메모리 레이아웃 계산해두는 함수. 이후 인스턴스를 생성할 때 이 캐시된 레이아웃으로 만듦.
// 인덱스는 파티클 생성할때 만들어 줄 거고 최대 파티클 개수같은 것은 인스턴스 생성할 때 Required 모듈이 결정함.
// 이 함수는 모듈의 상태가 아니라 이미터가 가진 모듈 타입 조합에만 의존해야함. 모듈 상태가 바뀐다고 메모리 레이아웃이 바뀌면 안됨.
// 스태틱 매시 컴포넌트의(인스턴스) Transform을 바꾸겠다고 UStaticMesh를 바꾸는 꼴임.
void UParticleEmitter::CacheEmitterModuleInfo()
{
	// 초기화
	ParticleSize = sizeof(FBaseParticle);
	ReqInstanceBytes = 0;
	ModuleOffsetMap.Empty();
	ModuleInstanceOffsetMap.Empty();


	if (LODLevels.Num() == 0)
		return;
	UParticleLODLevel* HightestLODLevel = LODLevels[0];

	UParticleModuleTypeDataBase* HighTypeData = HightestLODLevel->TypeDataModule;
	if (HighTypeData)
	{
		//.. 지금은 Sprite만 처리
		// TODO : Mesh, Beam 처리
	}

	// 모든 LOD Level이 같은 모듈 순서와 메모리 레이아웃을 사용한다고 가정

	for ( int32 ModuleIndex = 0; ModuleIndex < HightestLODLevel->Modules.Num() ; ModuleIndex++)
	{
		UParticleModule* ParticleModule = HightestLODLevel->Modules[ModuleIndex];
		// Module중에는 TypeData를 알아야 페이로드를 결정 가능한 모듈도 있음. 그래서 따로 처리함.
		if (ParticleModule->IsA(UParticleModuleTypeDataBase::StaticClass()) == false)
		{
			// 위와 같은 이유로 RequiredBytes에 인자로 TypeData를 넘기는데 아직은 스프라이트만 처리함
			int32 ReqBytes = ParticleModule->RequiredBytes(HighTypeData);
			if (ReqBytes)
			{
				ModuleOffsetMap.Add(ParticleModule, ParticleSize);
				ParticleSize += ReqBytes;
			}

			int32 TempInstanceBytes = ParticleModule->RequiredBytesPerInstance();
			if (TempInstanceBytes)
			{
				ModuleInstanceOffsetMap.Add(ParticleModule, ReqInstanceBytes);
				// LOD레벨 상관없이 똑같은 인스턴스 메모리 공유하므로 루프 안에선 해줄 필요 없음
				ModulesNeedingInstanceData.Add(ParticleModule);
				// 왜 인스턴스 데이터만 모든 LOD레벨의 모듈에 Offset 매핑을 해줄까?
				// => LOD가 자주 바뀌는데 그때마다 HightestLODLevel을 찾고 Module찾고 매핑하는 과정 오버헤드
				// + Payload의 경우 LOD 0레벨을 기준으로 메모리 레이아웃을 통일하고 있다는 사실을 명확히 하기 위함.
				for (int32 LODLevelIndex = 1; LODLevelIndex < LODLevels.Num(); LODLevelIndex++)
				{
					UParticleLODLevel* CurrentLODLevel = LODLevels[LODLevelIndex];
					ModuleInstanceOffsetMap.Add(CurrentLODLevel->Modules[ModuleIndex], ReqInstanceBytes);
				}
				ReqInstanceBytes += TempInstanceBytes;
			}
		}
	}
}

void UParticleEmitter::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	auto ResetLODLevels = [this]()
	{
		for (UParticleLODLevel* LOD : LODLevels)
		{
			if (LOD)
			{
				DeleteObject(LOD);
			}
		}
		LODLevels.Empty();
	};

	if (bInIsLoading)
	{
		ResetLODLevels();
		ModulesNeedingInstanceData.Empty();
		ModuleOffsetMap.Empty();
		ModuleInstanceOffsetMap.Empty();
		MeshMaterials.Empty();

		FJsonSerializer::ReadInt32(InOutHandle, "InitialAllocationCount", InitialAllocationCount, InitialAllocationCount, false);

		JSON MaterialsJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "MeshMaterials", MaterialsJson, nullptr, false))
		{
			for (uint32 MaterialIndex = 0; MaterialIndex < static_cast<uint32>(MaterialsJson.size()); ++MaterialIndex)
			{
				const JSON& MaterialValue = MaterialsJson.at(MaterialIndex);
				if (MaterialValue.JSONType() == JSON::Class::String)
				{
					FString Path = MaterialValue.ToString();
					if (!Path.empty())
					{
						MeshMaterials.Add(UResourceManager::GetInstance().Load<UMaterial>(Path));
					}
					else
					{
						MeshMaterials.Add(nullptr);
					}
				}
			}
		}

		JSON LODArrayJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "LODLevels", LODArrayJson, nullptr, false))
		{
			for (uint32 LODIndex = 0; LODIndex < static_cast<uint32>(LODArrayJson.size()); ++LODIndex)
			{
				JSON LODJson = LODArrayJson.at(LODIndex);

				FString TypeString;
				FJsonSerializer::ReadString(LODJson, "Type", TypeString, "UParticleLODLevel", false);

				UClass* LODClass = !TypeString.empty() ? UClass::FindClass(TypeString) : nullptr;
				if (!LODClass || !LODClass->IsChildOf(UParticleLODLevel::StaticClass()))
				{
					LODClass = UParticleLODLevel::StaticClass();
				}

				if (UParticleLODLevel* NewLOD = Cast<UParticleLODLevel>(ObjectFactory::NewObject(LODClass)))
				{
					NewLOD->Serialize(true, LODJson);
					LODLevels.Add(NewLOD);
				}
			}
		}

		CacheEmitterModuleInfo();
	}
	else
	{
		InOutHandle["InitialAllocationCount"] = InitialAllocationCount;

		JSON MaterialsJson = JSON::Make(JSON::Class::Array);
		for (UMaterialInterface* Material : MeshMaterials)
		{
			MaterialsJson.append(Material ? Material->GetFilePath().c_str() : "");
		}
		InOutHandle["MeshMaterials"] = MaterialsJson;

		JSON LODArrayJson = JSON::Make(JSON::Class::Array);
		for (UParticleLODLevel* LOD : LODLevels)
		{
			if (!LOD)
			{
				continue;
			}

			JSON LODJson = JSON::Make(JSON::Class::Object);
			LODJson["Type"] = LOD->GetClass()->Name;
			LOD->Serialize(false, LODJson);
			LODArrayJson.append(LODJson);
		}
		InOutHandle["LODLevels"] = LODArrayJson;
	}
}
