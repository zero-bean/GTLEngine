#pragma once
#include "ParticleModule.h"
#include "UParticleModuleTypeDataBase.generated.h"

UCLASS(DisplayName = "파티클 모듈 타입 데이터", Description = "")
class UParticleModuleTypeDataBase : public UParticleModule
{
	
public:

	GENERATED_REFLECTION_BODY()

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	UParticleModuleTypeDataBase() = default;
};