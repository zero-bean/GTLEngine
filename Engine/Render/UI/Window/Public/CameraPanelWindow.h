#pragma once
#include "Render/UI/Window/Public/UIWindow.h"

class Camera;

class UCameraPanelWindow : public UUIWindow
{
	using Super = UUIWindow;

public:
	UCameraPanelWindow();
	~UCameraPanelWindow() override {}

	virtual void Initialize() override;
	virtual void Render() override;

	/*
	 * @brief Setter 
	 */
	void SetCamera(Camera* InCamera) { CameraPtr = InCamera; }

private:
	void SyncFromCamera();
	void PushToCamera();

	/*
	 * @brief 멤버 변수 설명
	 * @var CameraModeIndex: 0: Perspective, 1: Orthographic
	 */
	Camera* CameraPtr = nullptr;
	float UiFovY = 80.f;
	float UiNearZ = 0.1f;
	float UiFarZ = 1000.f;
	int   CameraModeIndex = 0; 
};

