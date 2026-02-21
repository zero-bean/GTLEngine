#include "pch.h"
#include "Character.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "SkeletalMeshComponent.h"
#include "CapsuleComponent.h"
#include <windows.h>
#include <cmath>

ACharacter::ACharacter()
{
	// 기본 회전 속도 설정
	BaseTurnRate = 45.0f;

	// 캡슐 컴포넌트 생성 (충돌) - RootComponent로 설정
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	RootComponent = CapsuleComponent;

	// 스켈레탈 메시 컴포넌트 생성 (시각적 표현)
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>("CharacterMesh");
	// Mesh를 CapsuleComponent에 Attach - 이렇게 해야 Character 이동 시 함께 움직임
	Mesh->SetupAttachment(CapsuleComponent);
}

void ACharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateFacingFromMovement(DeltaTime);
}

void ACharacter::SetupPlayerInputComponent()
{
	Super::SetupPlayerInputComponent();

	// 이동 입력 바인딩 (WASD)
	InputComponent->BindAxis("MoveForward", VK_UP, 1.0f, this, &ACharacter::MoveForward);
	InputComponent->BindAxis("MoveForward", VK_DOWN, -1.0f, this, &ACharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", VK_RIGHT, 1.0f, this, &ACharacter::MoveRight);
	InputComponent->BindAxis("MoveRight", VK_LEFT, -1.0f, this, &ACharacter::MoveRight);
}

void ACharacter::MoveForward(float Value)
{
	PendingMovementInput.Y = Value;

	if (Value != 0.0f && MovementComponent)
	{
		// TODO: Transform의 Forward 방향으로 이동 입력 추가
		// 현재는 MovementComponent가 있으면 단순히 속도 설정
		FVector CurrentVelocity = MovementComponent->GetVelocity();
		// Forward 방향 (임시로 Y축 사용)
		CurrentVelocity.Y = Value * 100.0f;
		MovementComponent->SetVelocity(CurrentVelocity);
	}
}

void ACharacter::MoveRight(float Value)
{
	PendingMovementInput.X = Value;

	if (Value != 0.0f && MovementComponent)
	{
		// TODO: Transform의 Right 방향으로 이동 입력 추가
		// 현재는 MovementComponent가 있으면 단순히 속도 설정
		FVector CurrentVelocity = MovementComponent->GetVelocity();
		// Right 방향 (임시로 X축 사용)
		CurrentVelocity.X = Value * 100.0f;
		MovementComponent->SetVelocity(CurrentVelocity);
	}
}

void ACharacter::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	// 복사 시 OwnedComponents에서 컴포넌트를 찾아 멤버 변수 업데이트
	for (UActorComponent* Component : OwnedComponents)
	{
		if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Component))
		{
			Mesh = SkeletalMeshComp;
		}
		else if (UCapsuleComponent* CapsuleComp = Cast<UCapsuleComponent>(Component))
		{
			CapsuleComponent = CapsuleComp;
		}
	}
}

void ACharacter::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// 로딩 시 컴포넌트 복원
		CapsuleComponent = Cast<UCapsuleComponent>(RootComponent);

		for (UActorComponent* Component : OwnedComponents)
		{
			if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Component))
			{
				Mesh = SkeletalMeshComp;
				break;
			}
		}
	}
}

void ACharacter::UpdateFacingFromMovement(float DeltaTime)
{
	FVector DesiredDirection = DesiredFacingOverride;
	if (DesiredDirection.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		DesiredFacingOverride = FVector::Zero();
	}

	if (DesiredDirection.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		if (MovementComponent)
		{
			const FVector CurrentVelocity = MovementComponent->GetVelocity();
			if (CurrentVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				DesiredDirection = CurrentVelocity;
			}
		}

		if (DesiredDirection.SizeSquared() < KINDA_SMALL_NUMBER)
		{
			DesiredDirection = FVector(PendingMovementInput.X, PendingMovementInput.Y, 0.0f);
		}
	}

	DesiredDirection.Z = 0.0f;
	if (DesiredDirection.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}

	DesiredDirection.Normalize();
	const float DesiredYaw = RadiansToDegrees(std::atan2(DesiredDirection.Y, DesiredDirection.X));
	const FQuat TargetRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, DesiredYaw));
	const FQuat CurrentRotation = GetActorRotation();
	const float CurrentYaw = CurrentRotation.ToEulerZYXDeg().Z;
	const float DeltaYaw = NormalizeAngleDeg(DesiredYaw - CurrentYaw);

	// 일정 각도 이하로 접근하면 바로 스냅시켜 잔여 각도가 남지 않도록 처리
	if (FMath::Abs(DeltaYaw) <= 1.0f)
	{
		SetActorRotation(TargetRotation);
		return;
	}
	if (FacingInterpSpeed <= 0.0f)
	{
		SetActorRotation(TargetRotation);
		return;
	}
	const float Alpha = FMath::Clamp(DeltaTime * FacingInterpSpeed, 0.0f, 1.0f);
	const FQuat NewRotation = FQuat::Slerp(CurrentRotation, TargetRotation, Alpha);

	SetActorRotation(NewRotation);
}

void ACharacter::SetDesiredFacingDirection(const FVector& WorldDirection)
{
	DesiredFacingOverride = FVector(WorldDirection.X, WorldDirection.Y, 0.0f);
}
