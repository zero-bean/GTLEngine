#pragma once
#include <filesystem>
#include "Core/Public/Object.h"
#include "Core/Public/Delegate.h"
#include "Demo/Public/Player.h"
#include "Global/Types.h"

class UEditor;
class ULevel;
class AActor;
class UClass;

// Delegate for level change events
DECLARE_DELEGATE(FOnLevelChanged);

namespace json { class JSON; }
using JSON = json::JSON;

struct FWorldSettings
{
	UClass* DefaultPlayerClass = nullptr; // None: Free Camera Mode, Set: Game Mode with Player
};

// It represents the context in which the world is being used.
enum EWorldType
{
    Editor,        // A world being edited in the editor
	EditorPreview, // A preview world for an editor tool (e.g. static mesh viewer)
	PIE,           // A Play In Editor world (Cloned from Editor world)
	Game,          // The game world (Built and played by user)
};

/*
* <World>의 책임
* 1. 게임 전체의 Life Cycle 관리 (BeginPlay, EndPlay, Tick)
* 2. Tick 루프 조정 (Tick 순서 관리)
* 3. 레벨 관리 (레벨 load, save, reset) 트리거
* 4. Spawn, Destroy 시점 조정 (특히 Destroy 시점 관리. Destroy 요청 모았다가 안전한 시점에 처리할 수 있도록)
* 5. 월드 좌표계 기준 전역 쿼리(Octree, intersectoin test) 진입점
*/

// The World is the top level object representing a map or a sandbox in which Actors and Components will exist and be rendered.
UCLASS()
class UWorld final :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UWorld, UObject)

public:
	UWorld();
	UWorld(EWorldType WorldType);
	~UWorld();

	// Lifecycle & Tick
	void BeginPlay();
	bool EndPlay();
	void Tick(float DeltaTimes);
	float GetTimeSeconds() const { return WorldTimeSeconds; }
	uint32 GetFrameNumber() const { return FrameNumber; }

	// Level Management Triggers
	ULevel* GetLevel() const;
	void CreateNewLevel(const FName& InLevelName = FName::GetNone());
	bool LoadLevel(std::filesystem::path InLevelFilePath);
	bool SaveCurrentLevel(std::filesystem::path InLevelFilePath) const;

	// Actor Spawn & Destroy
	AActor* SpawnActor(UClass* InActorClass, JSON* ActorJsonData = nullptr);
	AActor* SpawnActor(const std::string& ClassName); // Helper for Lua binding (string overload)
	bool DestroyActor(AActor* InActor); // Level의 void MarkActorForDeletion(AActor * InActor) 기능을 DestroyActor가 가짐

	// Template Actor 검색 (Lua binding용 wrapper)
	AActor* FindTemplateActorOfName(const std::string& InName);
	/**
	 * @brief 월드(레벨)의 모든 액터를 순회하며 특정 클래스(및 자식 클래스)에 해당하는 액터들을 반환
	 * @param InClass 찾으려는 부모 클래스
	 * @return UClass*에 해당하는 AActor*의 TArray
	 */
	TArray<AActor*> FindActorsOfClass(UClass* InClass) const;

	/**
	 * @brief TArray<AActor*> FindActorsOfClass(UClass* InClass)의 템플릿 버전
	 * @tparam T 찾으려는 액터 타입
	 * @return T*의 TArray
	 */
	template<typename T>
	TArray<T*> FindActorsOfClass() const
	{
		TArray<AActor*> FoundActors = FindActorsOfClass(T::StaticClass());

		TArray<T*> CastedResult;
		CastedResult.Reserve(FoundActors.Num());

		for (AActor* Actor : FoundActors)
		{
			if (T* CastedActor = Cast<T>(Actor))
			{
				CastedResult.Add(CastedActor);
			}
		}
		return CastedResult;
	}

	EWorldType GetWorldType() const;
	void SetWorldType(EWorldType InWorldType);

	// Source Editor World 참조 (PIE World가 어느 Editor World로부터 복제되었는지 추적)
	UWorld* GetSourceEditorWorld() const { return SourceEditorWorld; }
	void SetSourceEditorWorld(UWorld* InSourceWorld) { SourceEditorWorld = InSourceWorld; }

	// Input Ignore
	void SetIgnoreInput(bool bInIgnore) { bIgnoreInput = bInIgnore; }
	bool IsIgnoringInput() const { return bIgnoreInput; }

	// Level change notification
	FOnLevelChanged OnLevelChanged;

private:
	EWorldType WorldType;
	ULevel* Level = nullptr; // Persistance Level. Sublevels are not considered in Engine.
	bool bBegunPlay = false;
	bool bIgnoreInput = false;
	TArray<AActor*> PendingDestroyActors;
	float WorldTimeSeconds;
	uint32 FrameNumber = 0;
	UWorld* SourceEditorWorld = nullptr; // PIE World가 복제된 원본 Editor World (Editor/Game world에서는 nullptr)

	void FlushPendingDestroy(); // Destroy marking 된 액터들을 실제 삭제

	void SwitchToLevel(ULevel* InNewLevel);

public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

// GameMode & Settings
public:
	class AGameMode* GetGameMode() const { return AuthorityGameMode; }
	const FWorldSettings& GetWorldSettings() const { return Settings; }
	void SetWorldSettings(const FWorldSettings& InWorldSettings) { Settings = InWorldSettings; }

private:
	AGameMode* AuthorityGameMode = nullptr;
	FWorldSettings Settings;
};
