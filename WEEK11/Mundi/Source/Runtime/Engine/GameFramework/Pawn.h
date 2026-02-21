#pragma once
#include "Actor.h"
#include "APawn.generated.h"

class AController;
class UInputComponent;
class UMovementComponent;

UCLASS(DisplayName = "폰", Description = "제어가 가능한 액터입니다.")
class APawn : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	APawn();

	// Controller 관계
	void SetController(AController* NewController) { Controller = NewController; }
	AController* GetController() const { return Controller; }

	// 입력 설정 - 서브클래스에서 오버라이드하여 입력 바인딩
	virtual void SetupPlayerInputComponent();

	// Tick 함수
	virtual void Tick(float DeltaTime) override;

	// 복사 및 직렬화
	void DuplicateSubObjects() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// 입력 컴포넌트
	UInputComponent* InputComponent = nullptr;
	// 이동 컴포넌트
	UMovementComponent* MovementComponent = nullptr;

protected:
	virtual ~APawn() override = default;

	// 현재 제어 중인 Controller
	AController* Controller = nullptr;
};

