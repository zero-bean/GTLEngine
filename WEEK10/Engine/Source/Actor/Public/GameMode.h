#pragma once
#include "Actor/Public/Actor.h"

DECLARE_DELEGATE(FOnGameInit);
DECLARE_DELEGATE(FOnGameStart);
DECLARE_DELEGATE(FOnGameEnd);
DECLARE_DELEGATE(FOnGameOver);

class APlayerCameraManager;

class AGameMode : public AActor
{
	DECLARE_CLASS(AGameMode, AActor)

// Delegate Section
public:
	FOnGameInit OnGameInited;
	FOnGameStart OnGameStarted;
	FOnGameEnd OnGameEnded;
	FOnGameEnd OnGameOvered;

public:
	AGameMode() = default;
	void BeginPlay() override;
	void EndPlay() override;
	void Tick(float DeltaTime) override;

	virtual void InitGame();
	virtual void StartGame();
	virtual void EndGame();
	virtual void OverGame();

	bool IsGameRunning() const { return bGameRunning; }
	bool IsGameEnded() const { return bGameEnded; }
	AActor* GetPlayer() const { return Player; }

	APlayerCameraManager* GetPlayerCameraManager() const { return PlayerCameraManager; }

protected:
	AActor* Player = nullptr;

private:
	// TODO: Move PlayerCameraManager ownership to APlayerController (like Unreal Engine)
	// Currently GameMode owns it temporarily until PlayerController is implemented
	APlayerCameraManager* PlayerCameraManager = nullptr;
	bool bGameRunning = false;
	bool bGameEnded = false;
};
