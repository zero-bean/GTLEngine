#pragma once
#include "World/Public/World.h"

class UEditor;
class ULevel;

UCLASS()
class UEditorEngine : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UEditorEngine, UObject)

public:
	UEditorEngine();
	~UEditorEngine();

	void Initialize();
	void Shutdown();

	virtual void BeginPlay();
	virtual void Tick(float DeltaSeconds);

	// World 관리
	UWorld* GetEditorWorld() const;
	UWorld* GetPIEWorld() const;
	UWorld* GetActiveWorld() const; // PIE 중이면 PIEWorld, 아니면 EditorWorld

	// Level 관리 (World에 위임)
	bool LoadLevel(const FString& InFilePath);
	bool SaveCurrentLevel(const FString& InFilePath);
	bool CreateNewLevel(const FString& InLevelName = "Untitled");
	ULevel* GetCurrentLevel() const;

	// PIE(Play In Editor) 관리
	void StartPIE();
	void EndPIE();
	bool IsPIEActive() const { return bPIEActive; }

	// Editor 접근
	UEditor* GetEditor() const { return Editor; }

private:
	UWorld* DuplicateWorldForPIE(UWorld* SourceWorld);
	FWorldContext* GetEditorWorldContext();
	FWorldContext* GetPIEWorldContext();


	TArray<FWorldContext> WorldContexts;
	UEditor* Editor = nullptr;
	bool bPIEActive = false;
};

// 글로벌 엔진 인스턴스
extern UEditorEngine* GEngine;
