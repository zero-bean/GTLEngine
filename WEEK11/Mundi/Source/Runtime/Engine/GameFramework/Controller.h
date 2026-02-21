#pragma once
#include "Actor.h"
#include "AController.generated.h"

class APawn;

UCLASS(DisplayName = "컨트롤러", Description = "액터의 제어를 명령하는 액터입니다.")
class AController : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	AController() = default;

	// Pawn 소유 관계
	virtual void Possess(APawn* InPawn);
	virtual void UnPossess();

	// Getter
	APawn* GetPawn() const { return Pawn; }

protected:
	virtual ~AController() override = default;

	// 현재 제어 중인 Pawn
	APawn* Pawn = nullptr;
};

