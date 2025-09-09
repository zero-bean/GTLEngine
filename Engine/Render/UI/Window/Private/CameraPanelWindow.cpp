#include "pch.h"
#include "Render/UI/Window/Public/CameraPanelWindow.h"
#include "Editor/Public/Camera.h"
#include "Manager/Input/Public/InputManager.h"

UCameraPanelWindow::UCameraPanelWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Camera Control";
	Config.DefaultSize = ImVec2(360, 420);
	Config.DefaultPosition = ImVec2(24, 80);
	Config.MinSize = ImVec2(320, 320);
	Config.DockDirection = EUIDockDirection::Left; // 필요시 변경
	Config.Priority = 10;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);
}

void UCameraPanelWindow::Initialize()
{
	// 필요 시 초기화 로그
	cout << "[UMousePanelWindow] Initialized\n";
}

void UCameraPanelWindow::SyncFromCamera()
{
	if (!CameraPtr) { return; }

	CameraModeIndex = (CameraPtr->GetCameraType() == ECameraType::ECT_Perspective) ? 0 : 1;
	UiFovY = CameraPtr->GetFovY();
	UiNearZ = CameraPtr->GetNearZ();
	UiFarZ = CameraPtr->GetFarZ();
}

void UCameraPanelWindow::PushToCamera()
{
	if (!CameraPtr) { return; }

	/*
	 * @brief 카메라 모드 설정
	 */
	CameraPtr->SetCameraType(CameraModeIndex == 0 ? ECameraType::ECT_Perspective
		: ECameraType::ECT_Orthographic);

	/*
	 * @brief 카메라 파라미터 설정
	 */
	UiNearZ = std::max(0.0001f, UiNearZ);
	UiFarZ = std::max(UiNearZ + 0.0001f, UiFarZ);
	UiFovY = std::min(170.0f, std::max(1.0f, UiFovY));

	CameraPtr->SetNearZ(UiNearZ);
	CameraPtr->SetFarZ(UiFarZ);
	CameraPtr->SetFovY(UiFovY);

	/*
	 * @brief 카메라 업데이트
	 */
	if (CameraModeIndex == 0)
	{
		CameraPtr->UpdateMatrixByPers();
	}
	else
	{
		CameraPtr->UpdateMatrixByOrth();
	}
}

void UCameraPanelWindow::Render()
{
	/*
	 * @brief 카메라 여부 검사
	 */
	if (!CameraPtr)
	{
		ImGui::TextUnformatted("Camera not set.");
		ImGui::Separator();
		ImGui::TextUnformatted("Call SetCamera(camera*) after creating this window.");
		return;
	}

	/*
	 * @brief 최초 1회 동기화
	 */
	static bool bSyncedOnce = false;
	if (bSyncedOnce == false) { SyncFromCamera(); bSyncedOnce = true; }

	// === Mouse (read-only)
	{
		ImGui::TextUnformatted("Mouse");
		ImGui::Separator();

		const UInputManager& Input = UInputManager::GetInstance();
		const FVector& Pos = Input.GetMousePosition();
		const FVector& Delta = Input.GetMouseDelta();

		ImGui::Text("Position:  x=%.1f  y=%.1f", Pos.X, Pos.Y);
		ImGui::Text("Delta:     dx=%.2f dy=%.2f", Delta.X, Delta.Y);
		ImGui::Spacing();
	}

	// === Camera Transform
	{
		ImGui::TextUnformatted("Camera Transform");
		ImGui::Separator();

		// 위치/회전은 참조로 직접 수정 (Camera가 참조 반환 제공)
		auto& Location = CameraPtr->GetLocation();
		auto& Rotation = CameraPtr->GetRotation();

		if (ImGui::DragFloat3("Position (X,Y,Z)", &Location.X, 0.05f)) {
			// 위치 바뀌면 바로 뷰 갱신
			if (CameraModeIndex == 0) CameraPtr->UpdateMatrixByPers();
			else                      CameraPtr->UpdateMatrixByOrth();
		}

		// 회전(도 단위) 입력 및 보정
		bool RotChanged = false;
		RotChanged |= ImGui::DragFloat3("Rotation (Pitch,Yaw,Roll)", &Rotation.X, 0.1f);

		// 피치/야오 간단 보정
		if (Rotation.X > 89.0f) Rotation.X = 89.0f;
		if (Rotation.X < -89.0f) Rotation.X = -89.0f;
		if (Rotation.Y > 180.0f) Rotation.Y -= 360.0f;
		if (Rotation.Y < -180.0f) Rotation.Y += 360.0f;

		if (RotChanged) {
			if (CameraModeIndex == 0) CameraPtr->UpdateMatrixByPers();
			else                      CameraPtr->UpdateMatrixByOrth();
		}

		ImGui::Spacing();
	}

	// === Camera Mode & Optics
	{
		ImGui::TextUnformatted("Camera Optics");
		ImGui::Separator();

		// 모드 전환
		const char* Models[] = { "Perspective", "Orthographic" };
		if (ImGui::Combo("Mode", &CameraModeIndex, Models, IM_ARRAYSIZE(Models))) {
			PushToCamera();
		}

		// FOV: 원근에서 주로 사용, 직교에서는 화면 폭에만 영향(너는 Ortho에서도 FovY로 OrthoWidth 계산함)
		// 카메라 코드에 맞춰 그대로 노출
		bool bChanged = false;
		bChanged |= ImGui::SliderFloat("FOV Y (deg)", &UiFovY, 1.0f, 170.0f, "%.1f");

		// Near/Far
		bChanged |= ImGui::DragFloat("Z Near", &UiNearZ, 0.01f, 0.0001f, 1e6f, "%.4f");
		bChanged |= ImGui::DragFloat("Z Far", &UiFarZ, 0.1f, 0.001f, 1e7f, "%.3f");

		if (bChanged) {
			PushToCamera();
		}

		// 퀵 버튼
		if (ImGui::Button("Reset Optics")) {
			UiFovY = 80.0f;
			UiNearZ = 0.1f;
			UiFarZ = 1000.0f;
			PushToCamera();
		}
	}
}
