#include "pch.h"
#include "JamesGameMode.h"
#include "PlayerController.h"
#include "JamesCharacter.h"
#include "World.h"

AJamesGameMode::AJamesGameMode()
{
	// 기본 설정
	ObjectName = "James Game Mode";
}

void AJamesGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG("[JamesGameMode] BeginPlay - bPie=%d", GetWorld()->bPie);

	// 1. PlayerController 스폰
	PlayerController = GetWorld()->SpawnActor<APlayerController>();
	UE_LOG("[JamesGameMode] PlayerController: %p", PlayerController);
	if (PlayerController)
	{
		PlayerController->SetActorLocation(FVector(0, 0, 0));
	}

	// 2. James 캐릭터 스폰
	PlayerCharacter = GetWorld()->SpawnActor<AJamesCharacter>();
	UE_LOG("[JamesGameMode] PlayerCharacter: %p", PlayerCharacter);
	if (PlayerCharacter)
	{
		// 캐릭터 초기 위치 설정 (Z=100으로 바닥 위에 배치)
		PlayerCharacter->SetActorLocation(FVector(0, 0, 0));
	}

	// 3. Controller와 Pawn 연결!
	if (PlayerController && PlayerCharacter)
	{
		UE_LOG("[JamesGameMode] Possessing");
		PlayerController->Possess(PlayerCharacter);

		PlayerCharacter->BeginPlay();
		PlayerController->BeginPlay();
	}
}

void AJamesGameMode::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}

void AJamesGameMode::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);
}
