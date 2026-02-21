#include "pch.h"
#include "JamesCharacter.h"
#include "SkeletalMeshComponent.h"
#include "CapsuleComponent.h"
#include "InputComponent.h"
#include "InputManager.h"
#include "AnimSequence.h"
#include <windows.h>

AJamesCharacter::AJamesCharacter()
{
	// James 스켈레탈 메시 설정
	if (Mesh)
	{
		Mesh->SetSkeletalMesh("Data/James/James.fbx");
		// TODO: 메시 위치 조정 (캡슐 중심에서 아래로)
	}

	// 캡슐 크기 설정
	if (CapsuleComponent)
	{
		// TODO: James 크기에 맞게 조정
	}
}

void AJamesCharacter::BeginPlay()
{
	Super::BeginPlay();

	// StateMachine 설정
	SetupAnimationStateMachine();
}

void AJamesCharacter::SetupPlayerInputComponent()
{
	Super::SetupPlayerInputComponent();

	InputComponent->BindAction("StartRun", VK_SHIFT, this, &AJamesCharacter::StartRunning);
}

void AJamesCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UInputManager& Input = UInputManager::GetInstance();
	if (Input.IsKeyReleased(VK_SHIFT))
	{
		StopRunning();
	}

	// 월드 축 기준으로 입력이 없으면 해당 축 속도를 제거 (Y=전/후, X=좌/우)
	if (!Input.IsKeyDown(VK_UP) && !Input.IsKeyDown(VK_DOWN))
	{
		CurrentVelocity.Y = 0.0f;
	}
	if (!Input.IsKeyDown(VK_LEFT) && !Input.IsKeyDown(VK_RIGHT))
	{
		CurrentVelocity.X = 0.0f;
	}

	SetDesiredFacingDirection(CurrentVelocity);

	float Speed = CurrentVelocity.Size();
	if (Speed > 0.01f)
	{
		FVector NewLocation = GetActorLocation() + CurrentVelocity * DeltaTime;
		SetActorLocation(NewLocation);
	}
}

void AJamesCharacter::MoveForward(float Value)
{
	Super::MoveForward(Value);

	float Speed = bIsRunning ? RunSpeed : WalkSpeed;
	CurrentVelocity.Y = Value * Speed;
}

void AJamesCharacter::MoveRight(float Value)
{
	Super::MoveRight(Value);

	float Speed = bIsRunning ? RunSpeed : WalkSpeed;
	CurrentVelocity.X = Value * Speed;
}

void AJamesCharacter::SetupAnimationStateMachine()
{
	if (!Mesh) { return; }

	// ───────────────────
	// 1. StateMachine에 상태(State) 추가
	// ───────────────────
	Mesh->AddSequenceInState("Idle", "Data/Animations/Breathing Idle.fbx", 0);
	Mesh->AddSequenceInState("Walk", "Data/Animations/Standard Walk.fbx", 0);
	Mesh->AddSequenceInState("Run", "Data/Animations/Standard Run.fbx", 0);

	// ───────────────────
	// 2. 상태 간 전환(Transition) 규칙 설정
	// ───────────────────

	// Idle → Walk: 속도가 있고 달리기 중이 아닐 때
	Mesh->AddTransition("Idle", "Walk", 0.2f, [this]() {
		float Speed = CurrentVelocity.Size();
		return Speed > 0.1f && !bIsRunning;
	});

	// Idle → Run: 속도가 있고 달리기 중일 때
	Mesh->AddTransition("Idle", "Run", 0.2f, [this]() {
		float Speed = CurrentVelocity.Size();
		return Speed > 0.1f && bIsRunning;
	});

	// Walk → Idle: 속도가 0일 때
	Mesh->AddTransition("Walk", "Idle", 0.2f, [this]() {
		float Speed = CurrentVelocity.Size();
		return Speed <= 0.1f;
	});

	// Walk → Run: 달리기 시작
	Mesh->AddTransition("Walk", "Run", 0.15f, [this]() {
		return bIsRunning;
	});

	// Run → Idle: 속도가 0일 때
	Mesh->AddTransition("Run", "Idle", 0.2f, [this]() {
		float Speed = CurrentVelocity.Size();
		return Speed <= 0.1f;
	});

	// Run → Walk: 달리기 중지
	Mesh->AddTransition("Run", "Walk", 0.15f, [this]() {
		float Speed = CurrentVelocity.Size();
		return Speed > 0.1f && !bIsRunning;
	});

	// ────────────────
	// 3. 시작 상태 설정 및 재생 시작
	// ────────────────
	Mesh->SetStartState("Idle");
	Mesh->Play();
}
