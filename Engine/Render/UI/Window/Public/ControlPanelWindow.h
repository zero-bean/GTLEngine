#pragma once
#include "UIWindow.h"

class UCamera;
class ULevelManager;
class AActor;

/**
* @brief Actor 속성을 편집하는 UI 윈도우
 * ImGuiManager의 Actor 편집 기능을 분리한 클래스
 */
class UControlPanelWindow : public UUIWindow
{
public:
	UControlPanelWindow();
	void Initialize() override;

	// Common Options
	void RenderPrimitiveSpawn();
	void RenderLevelIO();
	void RenderCamera();

	// FPS UI
	static ImVec4 GetFPSColor(float InFPS);

	// Level IO
	path OpenSaveFileDialog();
	path OpenLoadFileDialog();
	void SaveLevel(const FString& InFilePath);
	void LoadLevel(const FString& InFilePath);
	void CreateNewLevel();

	// Camera
	void SyncFromCamera();
	void PushToCamera();

	AActor* GetSelectedActor() const { return SelectedActor; }
	void SetSelectedActor(AActor* Actor) { SelectedActor = Actor; }

private:


	// Level IO
	ULevelManager* LevelManager;
	char NewLevelNameBuffer[256] = "Default";
	FString StatusMessage;
	float StatusMessageTimer = 0.0f;

	static constexpr float STATUS_MESSAGE_DURATION = 3.0f;

	// Camera
	UCamera* CameraPtr = nullptr;
	float UiFovY = 80.f;
	float UiNearZ = 0.1f;
	float UiFarZ = 1000.f;
	int   CameraModeIndex = 0;

	AActor* SelectedActor = nullptr;

	// Transform 편집
	FVector EditLocation;
	FVector EditRotation;
	FVector EditScale;

	// UI 상태
	bool bTransformChanged = false;

	//Level Memory Property
	int32 LevelObjectCount = 0;
	int32 LevelObjectMemory = 0;

	// Actor Spawn Property
	int32 SelectedPrimitiveType = 0; // 0: Cube, 1: Sphere, 2: Triangle
	int32 NumberOfSpawn = 1;
	float SpawnRangeMin = -5.0f;
	float SpawnRangeMax = 5.0f;

	// 편집 모드
	enum class EEditMode
	{
		Transform, // 트랜스폼 편집
		Components, // 컴포넌트 편집
		Properties // 속성 편집
	};

	EEditMode CurrentEditMode = EEditMode::Transform;

	void RenderTransformEditor();
	void RenderComponentEditor();
	void RenderPropertyEditor();
	void RenderActorInspection();
	void RenderModeSelector();

	// Actor Spawn & Delete
	void RenderLevelMemorySection();
	void RenderActorSpawnSection();
	void RenderActorDeleteSection();
	void SpawnActors() const;
	void DeleteSelectedActor();

	void UpdateTransformFromActor();
	void ApplyTransformToActor();
	bool HasValidActor() const { return SelectedActor != nullptr; }
};
