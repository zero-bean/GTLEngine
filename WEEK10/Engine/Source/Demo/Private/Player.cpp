#include "pch.h"
#include "Demo/Public/Player.h"

#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/BoxComponent.h"
#include "Component/Public/LightSensorComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Public/CameraComponent.h"
#include "Component/Public/SpringArmComponent.h"

IMPLEMENT_CLASS(APlayer, AActor)

APlayer::APlayer()
{
	bCanEverTick = true;
	CreateDefaultSubobject<UScriptComponent>()->SetScriptName("Player");
	SetCollisionTag(ECollisionTag::Player);
}

UClass* APlayer::GetDefaultRootComponent()
{
	return UBoxComponent::StaticClass();
}

void APlayer::InitializeComponents()
{
	Super::InitializeComponents();
	Cast<UBoxComponent>(GetRootComponent())->SetBoxExtent({1.0f, 1.0f, 1.0f});

	USpringArmComponent* SpringArm = CreateDefaultSubobject<USpringArmComponent>();
	SpringArm->AttachToComponent(GetRootComponent());
	SpringArm->SetTargetArmLength(30.0f);
	SpringArm->SetEnableCameraLag(true);
	SpringArm->SetCameraLagSpeed(5.0f);
	SpringArm->SetEnableArmLengthLag(true);
	SpringArm->SetArmLengthLagSpeed(100.0f);

	UCameraComponent* Camera = CreateDefaultSubobject<UCameraComponent>();
	Camera->AttachToComponent(SpringArm);

	ULightSensorComponent* LightSensor = CreateDefaultSubobject<ULightSensorComponent>();
	LightSensor->AttachToComponent(GetRootComponent());

	UStaticMeshComponent* StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>();
	StaticMesh->SetInheritYaw(true);
	StaticMesh->SetInheritPitch(false);
	StaticMesh->SetInheritRoll(false);

	StaticMesh->AttachToComponent(GetRootComponent());
	StaticMesh->SetStaticMesh("Data/Capsule.obj");
	StaticMesh->SetRelativeLocation(FVector{0, 0, -7});
	StaticMesh->SetRelativeScale3D(FVector{4, 4, 7});
}
