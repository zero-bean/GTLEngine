#pragma once
#include "Object.h"
#include "UParticleSystem.generated.h"

class UParticleEmitter;

UCLASS()
class UParticleSystem : public UObject
{
	GENERATED_REFLECTION_BODY()
public:
	TArray<UParticleEmitter*> Emitters;

	static UParticleSystem* TestParticleSystem;

	UParticleSystem();
	~UParticleSystem();

	static UParticleSystem* GetTestParticleSystem();

	static void ReleaseTestParticleSystem();

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
