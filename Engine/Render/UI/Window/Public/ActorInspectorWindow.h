#pragma once
#include "UIWindow.h"

class AActor;

/**
* @brief Actor 속성을 편집하는 UI 윈도우
 * ImGuiManager의 Actor 편집 기능을 분리한 클래스
 */
class UActorInspectorWindow : public UUIWindow
{
public:
	UActorInspectorWindow();
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
	bool bShowAdvancedOptions = false;

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
	void RenderModeSelector();

	void UpdateTransformFromActor();
	void ApplyTransformToActor();
	bool HasValidActor() const { return SelectedActor != nullptr; }
};
