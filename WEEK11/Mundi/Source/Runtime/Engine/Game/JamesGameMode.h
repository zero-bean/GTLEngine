#pragma once
#include "Actor.h"
#include "AJamesGameMode.generated.h"

class APlayerController;
class AJamesCharacter;

UCLASS(DisplayName = "제임스 게임모드", Description = "James 캐릭터를 위한 게임모드")
class AJamesGameMode : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	AJamesGameMode();
	virtual ~AJamesGameMode() override = default;

	// 게임 시작 시 PlayerController와 Pawn 생성 및 연결
	virtual void BeginPlay() override;

	// PlayerController와 Pawn 접근자
	APlayerController* GetPlayerController() const { return PlayerController; }
	AJamesCharacter* GetPlayerCharacter() const { return PlayerCharacter; }

	// 복사 및 직렬화
	void DuplicateSubObjects() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	// 생성된 액터들
	APlayerController* PlayerController = nullptr;
	AJamesCharacter* PlayerCharacter = nullptr;
};
