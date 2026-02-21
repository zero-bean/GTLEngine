#pragma once
#include "Global/BuildConfig.h"
#include "Subsystem/Public/EngineSubsystem.h"

class FAppWindow;
class UWorld;
class FViewport;
class UGameViewportClient;
class FSceneView;
class FSceneViewFamily;

/**
 * @brief GameInstance Subsystem (Runtime Only)
 * StandAlone 모드에서 게임의 전체 생명주기를 관리하는 서브시스템
 * Editor 모드에서는 UEditorEngine이 이 역할을 담당
 */
UCLASS()
class UGameInstance : public UEngineSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UGameInstance, UEngineSubsystem)

public:
	// UEngineSubsystem interface
	void Initialize() override;
	void Deinitialize() override;

	// GameInstance specific
	void InitializeWorld(FAppWindow* InWindow, const char* InScenePath = nullptr);
	void Tick(float DeltaTime);

	// Accessors
	UWorld* GetWorld() const { return World; }
	FViewport* GetViewport() const { return Viewport; }
	UGameViewportClient* GetViewportClient() const { return ViewportClient; }

	// SceneView 생성 (렌더링 시 사용)
	FSceneView* CreateSceneView() const;

	// Special member function
	UGameInstance();
	~UGameInstance() override = default;

private:
	FAppWindow* AppWindow = nullptr;

	// StandAlone World
	UWorld* World = nullptr;

	// Fullscreen Viewport
	FViewport* Viewport = nullptr;
	UGameViewportClient* ViewportClient = nullptr;
};

// Global GameInstance pointer
extern UGameInstance* GameInstance;
