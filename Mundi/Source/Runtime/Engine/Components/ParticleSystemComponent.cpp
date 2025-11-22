#include "pch.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystem.h"
#include "ParticleEmitterInstances.h"
#include "ParticleEmitter.h"

void UParticleSystemComponent::InitParticles()
{

	ResetParticles();

	if (Template.IsValid())
	{
		for (UParticleEmitter* Emitter : Template.Get()->Emitters)
		{
			FParticleEmitterInstance* NewInstance = Emitter->CreateInstance(this);

			NewInstance->InitParticles();
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
