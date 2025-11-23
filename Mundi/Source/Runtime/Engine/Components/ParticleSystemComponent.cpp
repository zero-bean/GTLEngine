#include "pch.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystem.h"
#include "ParticleEmitterInstances.h"
#include "ParticleEmitter.h"


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
	//테스트용 코드
	UParticleSystem* PS = UParticleSystem::GetTestParticleSystem();
	if (PS)
	{
		SetTemplate(PS);
	}
	
}

void UParticleSystemComponent::EndPlay()
{
	// 테스트용 코드
	UParticleSystem::ReleaseTestParticleSystem();
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
		if (EmitterInstance->HasComplete())
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

	for (FDynamicEmitterDataBase* RenderData : EmitterRenderDatas)
	{
		if (RenderData)
		{
			delete RenderData;
		}
	}
	EmitterRenderDatas.Empty();
}


