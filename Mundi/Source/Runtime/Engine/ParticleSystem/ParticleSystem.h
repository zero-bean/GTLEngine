#pragma once
#include "ResourceBase.h"
#include "UParticleSystem.generated.h"

class UParticleEmitter;

UCLASS()
class UParticleSystem : public UResourceBase
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

	bool Load(const FString& InFilePath, ID3D11Device* InDevice);
	bool Save(const FString& InFilePath) const;
};
