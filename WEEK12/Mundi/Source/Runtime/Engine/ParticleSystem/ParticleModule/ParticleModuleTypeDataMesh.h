#pragma once
#include "ParticleModuleTypeDataBase.h"
#include "UParticleModuleTypeDataMesh.generated.h"


UCLASS(DisplayName = "파티클 모듈 타입 메시", Description = "")
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{

public:
	GENERATED_REFLECTION_BODY()

	class UStaticMesh* StaticMesh = nullptr;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	
	UParticleModuleTypeDataMesh() = default;
};
