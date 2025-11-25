#pragma once
#include "PrimitiveComponent.h"
#include "UParticleSystemComponent.generated.h"

struct FParticleEmitterInstance;
class UParticleSystem;

UCLASS(DisplayName = "파티클 시스템 컴포넌트", Description = "파티클 시스템 컴포넌트")
class UParticleSystemComponent : public UPrimitiveComponent
{
public:

	GENERATED_REFLECTION_BODY()

	// 스폰 멈추기
	bool bSuppressSpawning = false;

	TArray<FParticleEmitterInstance*> EmitterInstances;

	UPROPERTY(EditAnywhere, Category = "Particle")
	UParticleSystem* Template = nullptr;

	void CollectMeshBatches(TArray<FMeshBatchElement>& MeshBatch, const FSceneView* View) override;

	UParticleSystemComponent();

	~UParticleSystemComponent();

	void SetTemplate(UParticleSystem* InTemplate);

	void DestroyEmitterInstances();

	void BeginPlay() override;

	void EndPlay() override;

	void TickComponent(float DeltaTime) override;

	void InitParticles();

	void ResetParticles();


};
