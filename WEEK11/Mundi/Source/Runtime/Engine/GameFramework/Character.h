#pragma once
#include "Pawn.h"
#include "Vector.h"
#include "ACharacter.generated.h"

class UMovementComponent;
class USkeletalMeshComponent;
class UCapsuleComponent;

UCLASS(DisplayName = "캐릭터", Description = "제어가 가능한 인간형 액터입니다.")
class ACharacter : public APawn
{
public:
	GENERATED_REFLECTION_BODY()

	ACharacter();

	// Character Tick drives facing alignment
	virtual void Tick(float DeltaTime) override;

	// 입력 설정 오버라이드
	virtual void SetupPlayerInputComponent() override;

	// 이동 함수들 - InputComponent에서 호출됨
	virtual void MoveForward(float Value);
	virtual void MoveRight(float Value);

	// 컴포넌트 접근자
	USkeletalMeshComponent* GetMesh() const { return Mesh; }
	UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

	// 복사
	void DuplicateSubObjects() override;

	// 직렬화
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	virtual ~ACharacter() override = default;

	// Orient the mesh using the override direction or the last movement input
	void UpdateFacingFromMovement(float DeltaTime);

	// 외부에서 원하는 바라보는 방향을 직접 지정할 수 있도록 지원
	void SetDesiredFacingDirection(const FVector& WorldDirection);

	// 이동 속성
	float BaseTurnRate = 45.0f;		// 초당 도 단위

	// 입력 축을 저장하여 이동 방향을 계산합니다.
	FVector2D PendingMovementInput = FVector2D(0.0f, 0.0f);
	FVector DesiredFacingOverride = FVector::Zero();

	UPROPERTY(EditAnywhere, Category = "Movement")
	float FacingInterpSpeed = 10.0f;

	// 캐릭터 컴포넌트들
	USkeletalMeshComponent* Mesh = nullptr;
	UCapsuleComponent* CapsuleComponent = nullptr;
};

