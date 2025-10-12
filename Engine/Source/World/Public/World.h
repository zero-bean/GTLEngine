#pragma once
#include "Core/Public/Object.h"
#include "Core/Public/ObjectPtr.h"
#include "Actor/Public/Actor.h"
#include "Level/Public/Level.h"

class UWorld;

enum class EWorldType : uint8
{
	None,
	Editor,
	EditorPreview,
	PIE,
	Game,
};

inline FString to_string(EWorldType e)
{
	switch (e)
	{
	case EWorldType::None: return "None";
	case EWorldType::Editor: return "Editor";
	case EWorldType::EditorPreview: return "EditorPreview";
	case EWorldType::PIE: return "PIE";
	case EWorldType::Game: return "Game";
	}
	return "unknown";
}


struct FWorldContext
{
	FName ContextHandle{};
	EWorldType WorldType{EWorldType::None};
	TObjectPtr<UWorld> WorldPtr;
	TObjectPtr<UWorld> World() const { return WorldPtr; }
};


UCLASS()

class UWorld : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UWorld, UObject)

public:
	UWorld();
	~UWorld() override;

	virtual void Tick(float DeltaSeconds);

	bool LoadLevel(const FString& InFilePath);
	bool SaveLevel(const FString& InFilePath);
	bool CreateNewLevel(const FString& InLevelName = "Untitled");

	ULevel* GetLevel() const { return Level; }
	void SetLevel(ULevel* InLevel);

	EWorldType GetWorldType() const { return WorldType; }
	void SetWorldType(EWorldType InWorldType) { WorldType = InWorldType; }


	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;

private:
	ULevel* Level;
	EWorldType WorldType{EWorldType::None};

	FFrustumCull* Frustum;

	void SwitchToLevel(ULevel* InNewLevel);
	static path GetLevelDirectory();
	static path GenerateLevelFilePath(const FString& InLevelName);

	void ProcessInput();

	bool bSpawnDecalOnClick = true;
};
