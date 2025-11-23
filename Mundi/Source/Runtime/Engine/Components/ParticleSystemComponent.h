#pragma once
#include "PrimitiveComponent.h"
#include "UParticleSystemComponent.generated.h"

struct FParticleEmitterInstance;
struct FDynamicEmitterDataBase;
class UParticleSystem;

UCLASS(DisplayName = "파티클 시스템 컴포넌트", Description = "파티클 시스템 컴포넌트")
class UParticleSystemComponent : public UPrimitiveComponent
{
public:

	GENERATED_REFLECTION_BODY()

	// 스폰 멈추기
	bool bSuppressSpawning = false;

	UParticleSystemComponent();

	~UParticleSystemComponent();

	void DestroyEmitterInstances();

	void BeginPlay() override;

	void TickComponent(float DeltaTime) override;

	void InitParticles();

	void ResetParticles();

	
private:

	TArray<FParticleEmitterInstance*> EmitterInstances;

	TWeakObjectPtr<UParticleSystem> Template = nullptr;

	TArray<FDynamicEmitterDataBase*> EmitterRenderDatas;
};
