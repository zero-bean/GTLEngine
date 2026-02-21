#include "pch.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/PlayerStart.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Actor/Public/CameraActor.h"
#include "Component/Public/CameraComponent.h"
#include "Level/Public/Level.h"

IMPLEMENT_CLASS(AGameMode, AActor)

void AGameMode::BeginPlay()
{
	if (bBegunPlay) return;

	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG_ERROR("GameMode::BeginPlay: GetWorld() returned nullptr");
		return;
	}

	// TODO: Move to APlayerController::SpawnPlayerCameraManager() when PlayerController is implemented
	// Find existing PlayerCameraManager in level, or create new one
	TArray<APlayerCameraManager*> CameraManagers = World->FindActorsOfClass<APlayerCameraManager>();
	if (CameraManagers.Num() > 0)
	{
		// Use first PlayerCameraManager found in level
		PlayerCameraManager = CameraManagers[0];
		UE_LOG_INFO("GameMode: Using existing PlayerCameraManager '%s' from level",
			PlayerCameraManager->GetName().ToString().c_str());
	}
	else
	{
		// Create new PlayerCameraManager
		PlayerCameraManager = Cast<APlayerCameraManager>(World->SpawnActor(APlayerCameraManager::StaticClass()));
		UE_LOG_INFO("GameMode: Created new PlayerCameraManager");
	}

	if (PlayerCameraManager)
	{
		PlayerCameraManager->BeginPlay();
	}

	const FWorldSettings& WorldSettings = World->GetWorldSettings();
	if (WorldSettings.DefaultPlayerClass)
	{
		TArray<APlayerStart*> PlayerStarts = World->FindActorsOfClass<APlayerStart>();

		Player = World->SpawnActor(WorldSettings.DefaultPlayerClass);

		if (PlayerStarts.Num() > 0)
		{
			// SpawnActor Location 인자 있는 버전으로 바뀌면 변경
			Player->SetActorLocation(PlayerStarts[0]->GetActorLocation());
			Player->SetActorRotation(PlayerStarts[0]->GetActorRotation());
		}
	}

	// Set ViewTarget for camera system
	if (PlayerCameraManager)
	{
		// Player 스폰에 성공했다면, ViewTarget은 Player
		if (Player)
		{
			PlayerCameraManager->SetViewTarget(Player);
			UE_LOG_INFO("GameMode: Set ViewTarget to spawned Player '%s'",
			   Player->GetName().ToString().c_str());
		}
		// 만약 Player 스폰에 실패했다면, 기본 카메라를 스폰
		else
		{
			UE_LOG("GameMode: Player spawning failed. Creating default spectator camera.");
			ACameraActor* DefaultCamera = Cast<ACameraActor>(World->SpawnActor(ACameraActor::StaticClass()));
			if (DefaultCamera)
			{
				DefaultCamera->SetActorLocation(FVector(0, 0, 0));
				DefaultCamera->SetActorRotation(FQuaternion::Identity());
				PlayerCameraManager->SetViewTarget(DefaultCamera);
				UE_LOG_INFO("GameMode: Created default CameraActor at origin as ViewTarget");
			}
			else
			{
				UE_LOG_ERROR("GameMode: Failed to create default camera ViewTarget");
			}
		}
	}

	InitGame();
}

void AGameMode::EndPlay()
{
	Super::EndPlay();

	if (PlayerCameraManager)
	{
		PlayerCameraManager->SetViewTarget(nullptr);
	}

	// Player 포인터 정리 (게임 재시작 시 dangling pointer 방지)
	Player = nullptr;
	PlayerCameraManager = nullptr;
}

void AGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// PlayerCameraManager updates itself via its own Tick
}

void AGameMode::InitGame()
{
	bGameRunning = false;
	bGameEnded = false;
	OnGameInited.Broadcast();
}

void AGameMode::StartGame()
{
	bGameRunning = true;
	bGameEnded = false;
	OnGameStarted.Broadcast();
}

void AGameMode::EndGame()
{
    // Prevent duplicate end-game broadcasts
    if (bGameEnded) {
        return;
    }
    bGameRunning = false;
    bGameEnded = true;
    OnGameEnded.Broadcast();
}

void AGameMode::OverGame()
{
	if (bGameEnded) {
		return;
	}
	
	bGameRunning = false;

	OnGameOvered.Broadcast();
}
