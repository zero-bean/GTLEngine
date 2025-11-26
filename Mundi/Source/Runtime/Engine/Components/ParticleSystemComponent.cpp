#include "pch.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystem.h"
#include "ParticleEmitterInstances.h"
#include "ParticleEmitter.h"
#include "MeshBatchElement.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"
#include "Material.h"
#include "Texture.h"


void UParticleSystemComponent::CollectMeshBatches(TArray<FMeshBatchElement>& MeshBatch, const FSceneView* View)
{
	for (FParticleEmitterInstance* EmitterInstance : EmitterInstances)
	{
		EmitterInstance->FillMeshBatch(MeshBatch, View);
	}
}

UParticleSystemComponent::UParticleSystemComponent()
{
	bCanEverTick = true;
}

UParticleSystemComponent::~UParticleSystemComponent()
{
	DestroyEmitterInstances();
}

void UParticleSystemComponent::SetTemplate(UParticleSystem* InTemplate)
{
	if (Template == InTemplate)
	{
		return;
	}

	if (InTemplate)
	{
		DestroyEmitterInstances();

		Template = InTemplate;

		InitParticles();
	}

}

void UParticleSystemComponent::LoadParticleSystemFromAssetPath()
{
	// 기존 Template 정리
	if (Template)
	{
		DeleteObject(Template);
		Template = nullptr;
	}

	// ParticleSystemAssetPath가 비어있으면 (None 선택) Template을 nullptr로 유지
	if (ParticleSystemAssetPath.empty())
	{
		// 파티클 인스턴스도 정리
		DestroyEmitterInstances();
		return;
	}

	// 새 파티클 시스템 로드
	UParticleSystem* NewSystem = NewObject<UParticleSystem>();
	if (NewSystem && NewSystem->Load(ParticleSystemAssetPath, nullptr))
	{
		Template = NewSystem;
		InitParticles();
	}
	else
	{
		if (NewSystem)
		{
			DeleteObject(NewSystem);
		}
	}
}

void UParticleSystemComponent::DestroyEmitterInstances()
{
	for (int32 Index = 0; Index < EmitterInstances.Num(); Index++)
	{
		if (EmitterInstances[Index])
		{
			delete EmitterInstances[Index];
		}
	}
	EmitterInstances.Empty();
}

void UParticleSystemComponent::BeginPlay()
{
	// AssetPath가 있으면 먼저 로드
	if (!ParticleSystemAssetPath.empty())
	{
		LoadParticleSystemFromAssetPath();
	}
	// Template이 설정되어 있으면 파티클 초기화
	else if (Template)
	{
		InitParticles();
	}
}

void UParticleSystemComponent::EndPlay()
{
	DestroyEmitterInstances();
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
	for (int32 EmitterIndex = 0 ; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
	{
		FParticleEmitterInstance* EmitterInstance = EmitterInstances[EmitterIndex];
		if (EmitterInstance)
		{
			EmitterInstance->Tick(DeltaTime, bSuppressSpawning);
		}
		if (EmitterInstance && EmitterInstance->HasComplete())
		{
			delete EmitterInstance;
			EmitterInstances[EmitterIndex] = nullptr;
		}
	}
}


void UParticleSystemComponent::InitParticles()
{
	ResetParticles();

	if (Template)
	{
		for (UParticleEmitter* Emitter : Template->Emitters)
		{
			FParticleEmitterInstance* NewInstance = Emitter->CreateInstance(this);

			// Required Module에서 Material의 텍스처 가져오기
			if (Emitter->LODLevels.Num() > 0 && Emitter->LODLevels[0])
			{
				UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
				if (LODLevel->RequiredModule && LODLevel->RequiredModule->Material)
				{
					UTexture* DiffuseTexture = LODLevel->RequiredModule->Material->GetTexture(EMaterialTextureSlot::Diffuse);
					if (DiffuseTexture && DiffuseTexture->GetShaderResourceView())
					{
						NewInstance->InstanceSRV = DiffuseTexture->GetShaderResourceView();
					}
				}
			}

			NewInstance->Init();
			EmitterInstances.Add(NewInstance);
		}
	}
}

void UParticleSystemComponent::ResetParticles()
{
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (Instance)
		{
			delete Instance;
		}
	}

	EmitterInstances.Empty();
}