#include "pch.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystem.h"
#include "ParticleEmitterInstances.h"
#include "ParticleEmitter.h"


UParticleSystemComponent::UParticleSystemComponent()
{
}

UParticleSystemComponent::~UParticleSystemComponent()
{
	DestroyEmitterInstances();
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

	if (Template.IsValid())
	{
		for (UParticleEmitter* Emitter : Template.Get()->Emitters)
		{
			FParticleEmitterInstance* NewInstance = Emitter->CreateInstance(this);

			NewInstance->Init();
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


