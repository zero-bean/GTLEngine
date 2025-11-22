#pragma once
#include "PrimitiveComponent.h"

struct FParticleEmitterInstance;
struct FDynamicEmitterDataBase;
class UParticleSystem;

class UParticleSystemComponent : public UPrimitiveComponent
{
public:

	void InitParticles();

	void ResetParticles();

private:

	TArray<FParticleEmitterInstance*> EmitterInstances;

	TWeakObjectPtr<UParticleSystem> Template = nullptr;

	TArray<FDynamicEmitterDataBase*> EmitterRenderDatas;
};
