#pragma once
#include "ParticleModuleTypeDataBase.h"
#include "UParticleModuleTypeDataMesh.generated.h"


UCLASS(DisplayName = "파티클 모듈 타입 메시", Description = "")
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{

public:
	class UStaticMesh* StaticMesh = nullptr;

	GENERATED_REFLECTION_BODY()
	UParticleModuleTypeDataMesh() = default;
};
