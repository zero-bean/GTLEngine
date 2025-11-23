#pragma once
#include "ParticleModule.h"
#include "UParticleModuleRequired.generated.h"

class UMaterialInterface;

UCLASS()
class UParticleModuleRequired : public UParticleModule
{
	GENERATED_REFLECTION_BODY()
public:


	UMaterialInterface* Material;

	// 이미터 1회 루프 시간
	float EmitterDuration = 0;

	// 이미터 시작 전 대기시간
	float EmitterDelay = 0;

	// 이미터 반복 횟수, 0: 무한 1: 1회 ...
	int32 EmitterLoops = 0;

	// 이미터가 생성된 이후 부모 컴포넌트를 따라다닐지, 제자리에 남을지 결정.
	// ex) true인 경우: 촛불이 꽃힌 케이크가 움직일때 불꽃도 같이 케이크 따라 움직임
	//     false인 경우: 자동차 배기가스는 분출된 이후 차를 따라가지 않음.
	bool bUseLocalSpace = true;

	UParticleModuleRequired() = default;
};