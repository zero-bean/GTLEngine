#pragma once
#include "Character.h"
#include "AJamesCharacter.generated.h"

class UAnimSequence;

UCLASS(DisplayName = "제임스 캐릭터", Description = "James.fbx를 사용하는 샘플 캐릭터")
class AJamesCharacter : public ACharacter
{
public:
	GENERATED_REFLECTION_BODY()

	AJamesCharacter();

	// 게임 시작 시 호출
	void BeginPlay() override;

	// 입력 설정
	void SetupPlayerInputComponent() override;

	// 매 프레임 호출
	void Tick(float DeltaTime) override;

	// 이동 함수 
	void MoveForward(float Value) override;
	void MoveRight(float Value) override;
	void StartRunning() { bIsRunning = true; }
	void StopRunning() { bIsRunning = false; }

protected:
	virtual ~AJamesCharacter() override = default;

	// 이동 속도
	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float RunSpeed = 4.0f;

	bool bIsRunning = false;

	// 현재 속도
	FVector CurrentVelocity;

private:
	// StateMachine 설정 헬퍼 함수
	void SetupAnimationStateMachine();
};
