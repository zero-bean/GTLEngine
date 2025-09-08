#pragma once
#include "UIWindow.h"

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
	void Render() override;

	AActor* GetSelectedActor() const { return SelectedActor; }
	void SetSelectedActor(AActor* Actor) { SelectedActor = Actor; }

private:
	AActor* SelectedActor = nullptr;

	// Transform 편집
	FVector EditLocation;
	FVector EditRotation;
	FVector EditScale;

	// UI 상태
	bool bTransformChanged = false;

	// Actor Spawn Property
	int SelectedPrimitiveType = 0; // 0: Cube, 1: Sphere, 2: Triangle
	int NumberOfSpawn = 1;
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
	void RenderActorSpawnSection();
	void RenderActorDeleteSection();
	void SpawnActors();
	void DeleteSelectedActor();

	void UpdateTransformFromActor();
	void ApplyTransformToActor();
	bool HasValidActor() const { return SelectedActor != nullptr; }
};
