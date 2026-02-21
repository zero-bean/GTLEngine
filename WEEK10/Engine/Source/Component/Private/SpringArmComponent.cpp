#include "pch.h"
#include "Component/Public/SpringArmComponent.h"

#include "Level/Public/Level.h"
#include "Physics/Public/BoundingSphere.h"
#include "Utility/Public/JsonSerializer.h"
IMPLEMENT_CLASS(USpringArmComponent, USceneComponent)


USpringArmComponent::USpringArmComponent()
{
	TargetArmLength = 300.0f;
	bEnableCameraLag = false;
	CameraLagSpeed = 10.0f;

	bEnableArmLengthLag = false;
	ArmLengthLagSpeed = 100.0f;

	PreviousLocation = FVector::ZeroVector();
	bIsFirstUpdate = true;
	bCanEverTick = true;
}

void USpringArmComponent::BeginPlay()
{
	Super::BeginPlay();

	// 초기 위치 설정
	CurrentArmLength = TargetArmLength;
	PreviousLocation = GetWorldLocation() + FVector(-TargetArmLength, 0.0f, 0.0f);
	bIsFirstUpdate = true;
}

void USpringArmComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	UpdateCamera(DeltaTime);
	bIsFirstUpdate = false;
}

UObject* USpringArmComponent::Duplicate()
{
	USpringArmComponent* NewSpringArm = Cast<USpringArmComponent>(Super::Duplicate());
	NewSpringArm->TargetArmLength = TargetArmLength;
	NewSpringArm->bEnableCameraLag = bEnableCameraLag;
	NewSpringArm->CameraLagSpeed = CameraLagSpeed;
	NewSpringArm->bEnableArmLengthLag = bEnableArmLengthLag;
	NewSpringArm->ArmLengthLagSpeed = ArmLengthLagSpeed;
	NewSpringArm->CurrentArmLength = CurrentArmLength;
	NewSpringArm->bDoCollisionTest = bDoCollisionTest;

	return NewSpringArm;
}

void USpringArmComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "TargetArmLength", TargetArmLength, 0);
		FJsonSerializer::ReadBool(InOutHandle, "bEnableCameraLag", bEnableCameraLag, false);
		FJsonSerializer::ReadFloat(InOutHandle, "CameraLagSpeed", CameraLagSpeed, 10);
		FJsonSerializer::ReadBool(InOutHandle, "bEnableArmLengthLag", bEnableArmLengthLag, false);
		FJsonSerializer::ReadBool(InOutHandle, "bDoCollisionTest", bDoCollisionTest, true);
		FJsonSerializer::ReadFloat(InOutHandle, "ArmLengthLagSpeed", ArmLengthLagSpeed, 100);
	}
	// 저장
	else
	{
		InOutHandle["TargetArmLength"] = TargetArmLength;
		InOutHandle["CameraLagSpeed"] = CameraLagSpeed;
		InOutHandle["ArmLengthLagSpeed"] = ArmLengthLagSpeed;

		InOutHandle["bEnableCameraLag"] = bEnableCameraLag ? "true" : "false";
		InOutHandle["bEnableArmLengthLag"] = bEnableArmLengthLag ? "true" : "false";
		InOutHandle["bDoCollisionTest"] = bDoCollisionTest ? "true" : "false";
	}
}

void USpringArmComponent::UpdateCamera(float DeltaTime)
{
	ULevel* Level = GWorld ? GWorld->GetLevel() : nullptr;
	if (!Level) { return; }

	if (bEnableArmLengthLag && !bIsFirstUpdate)
	{
		float Alpha = Clamp(ArmLengthLagSpeed * DeltaTime, 0.0f, 1.0f);
		CurrentArmLength = Lerp(CurrentArmLength, TargetArmLength, Alpha);
	}
	else
	{
		CurrentArmLength = TargetArmLength;
	}

	FVector Start = GetWorldLocation();
	FVector IdealEnd = Start - GetForwardVector() * CurrentArmLength;
	FVector FinalLocation = IdealEnd;

	FHitResult Hit;
	if (Level->LineTraceSingle(Start, IdealEnd, Hit, GetOwner(), ECollisionTag::Score))
	{
		constexpr float SafeDistance = -10.0f;
		FVector Dir = (IdealEnd - Start).GetNormalized();
		FinalLocation = Hit.Location + Dir * SafeDistance;

		// ArmLength를 실제 충돌 지점까지로 갱신
		float NewLength = (FinalLocation - Start).Length();
		CurrentArmLength = min(CurrentArmLength, NewLength);
	}

	if (bEnableCameraLag && !bIsFirstUpdate)
	{
		float Alpha = Clamp(CameraLagSpeed * DeltaTime, 0.0f, 1.0f);
		FinalLocation = FVector::Lerp(PreviousLocation, FinalLocation, Alpha);
	}

	const TArray<USceneComponent*>& Children = GetChildren();
	for (USceneComponent* Child : Children)
	{
		if (Child)
		{
			Child->SetWorldLocation(FinalLocation);
		}
	}

	PreviousLocation = FinalLocation;

}
