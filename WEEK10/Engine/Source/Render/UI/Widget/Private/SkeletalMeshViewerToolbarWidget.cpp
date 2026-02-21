#include "pch.h"
#include "Render/UI/Widget/Public/SkeletalMeshViewerToolbarWidget.h"
#include "Render/UI/Window/Public/SkeletalMeshViewerWindow.h"
#include "ImGui/imgui_internal.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Editor/Public/Camera.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Texture/Public/Texture.h"

IMPLEMENT_CLASS(USkeletalMeshViewerToolbarWidget, UViewportToolbarWidgetBase)

/**
 * @brief SkeletalMeshViewerToolbarWidget 생성자
 */
USkeletalMeshViewerToolbarWidget::USkeletalMeshViewerToolbarWidget()
	: UViewportToolbarWidgetBase()
{
	SetName("Skeletal Mesh Viewer Toolbar Widget");
}

/**
 * @brief 소멸자
 */
USkeletalMeshViewerToolbarWidget::~USkeletalMeshViewerToolbarWidget()
{
}

/**
 * @brief 초기화
 */
void USkeletalMeshViewerToolbarWidget::Initialize()
{
	LoadCommonIcons();
}

/**
 * @brief 업데이트
 */
void USkeletalMeshViewerToolbarWidget::Update()
{
	// QWER 키보드 단축키 처리 (뷰어 윈도우가 포커스되어 있을 때)
	// 우클릭이 눌러져 있지 않을 때만 동작 (카메라 이동과 충돌 방지)
	if (ImGui::IsWindowFocused() && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Q))
		{
			bSelectModeActive = true;
			CurrentGizmoMode = EGizmoMode::Translate; // Select 모드 활성화 시 기본 변환 모드
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_W))
		{
			bSelectModeActive = false;
			CurrentGizmoMode = EGizmoMode::Translate;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_E))
		{
			bSelectModeActive = false;
			CurrentGizmoMode = EGizmoMode::Rotate;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_R))
		{
			bSelectModeActive = false;
			CurrentGizmoMode = EGizmoMode::Scale;
		}
	}
}

// ========================================
// Viewport MenuBar Helper Functions (제거됨 - 베이스 클래스 사용)
// ========================================

/**
 * @brief 뷰포트 툴바 렌더링 (메인 에디터 뷰포트와 동일한 디자인)
 * 좌측: Gizmo Mode 아이콘 (비활성화) + Rotation Snap (비활성화)
 * 우측: ViewType + Camera Settings + View Mode
 */
void USkeletalMeshViewerToolbarWidget::RenderWidget()
{
	if (!ViewportClient)
	{
		return;
	}

	UCamera* Camera = ViewportClient->GetCamera();
	if (!Camera)
	{
		return;
	}

	// ViewMode labels
	static const char* ViewModeLabels[] = {
		"Gouraud", "Lambert", "BlinnPhong", "Unlit", "Wireframe", "SceneDepth", "WorldNormal"
	};

	// ViewType labels
	static const char* ViewTypeLabels[] = {
		"Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back"
	};

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));

	// ========================================
	// Left Side: Gizmo Mode Buttons
	// ========================================

	// 베이스 클래스의 Gizmo Mode 버튼 렌더링
	EGizmoMode OutMode;
	bool OutSelect;
	RenderGizmoModeButtons(CurrentGizmoMode, bSelectModeActive, OutMode, OutSelect);
	CurrentGizmoMode = OutMode;
	bSelectModeActive = OutSelect;

	// ========================================
	// World/Local Space Toggle
	// ========================================

	// QWER 버튼 다음에 8px 간격
	ImGui::SameLine(0.0f, 8.0f);

	// 베이스 클래스의 World/Local 토글 렌더링
	if (OwningWindow)
	{
		UGizmo* ViewerGizmo = OwningWindow->GetGizmo();
		if (ViewerGizmo)
		{
			RenderWorldLocalToggle(ViewerGizmo);
		}
	}

	// ========================================
	// Location Snap
	// ========================================

	// 베이스 클래스의 Location Snap 컨트롤 렌더링
	float OldLocationSnapValue = LocationSnapValue;
	RenderLocationSnapControls(bLocationSnapEnabled, LocationSnapValue);

	// Location Snap 값이 변경되면 Grid 크기 동기화
	if (OwningWindow && LocationSnapValue != OldLocationSnapValue)
	{
		OwningWindow->SetGridCellSize(LocationSnapValue);
	}

	// ========================================
	// Rotation Snap
	// ========================================

	// 베이스 클래스의 Rotation Snap 컨트롤 렌더링
	RenderRotationSnapControls(bRotationSnapEnabled, RotationSnapAngle);

	// ========================================
	// Scale Snap
	// ========================================

	// 베이스 클래스의 Scale Snap 컨트롤 렌더링
	RenderScaleSnapControls(bScaleSnapEnabled, ScaleSnapValue);

	// ========================================
	// Right Side: ViewType, Camera Settings, ViewMode
	// ========================================

	// Calculate right-aligned position
	EViewType CurrentViewType = ViewportClient->GetViewType();
	int32 CurrentViewTypeIndex = static_cast<int32>(CurrentViewType);
	constexpr float RightViewTypeButtonWidthDefault = 110.0f;
	constexpr float RightViewTypeIconSize = 16.0f;
	constexpr float RightViewTypePadding = 4.0f;

	EViewModeIndex CurrentMode = ViewportClient->GetViewMode();
	int32 CurrentModeIndex = static_cast<int32>(CurrentMode);
	constexpr float ViewModeButtonHeight = 24.0f;
	constexpr float ViewModeIconSize = 16.0f;
	constexpr float ViewModePadding = 4.0f;
	const ImVec2 ViewModeTextSize = ImGui::CalcTextSize(ViewModeLabels[CurrentModeIndex]);
	const float ViewModeButtonWidth = ViewModePadding + ViewModeIconSize + ViewModePadding + ViewModeTextSize.x + ViewModePadding;

	constexpr float CameraSpeedButtonWidth = 70.0f;
	constexpr float RightButtonSpacing = 6.0f;
	const float TotalRightButtonsWidth = RightViewTypeButtonWidthDefault + RightButtonSpacing + CameraSpeedButtonWidth + RightButtonSpacing + ViewModeButtonWidth;

	{
		const float ContentRegionRight = ImGui::GetWindowContentRegionMax().x;
		float RightAlignedX = ContentRegionRight - TotalRightButtonsWidth - 6.0f;
		RightAlignedX = std::max(RightAlignedX, ImGui::GetCursorPosX());

		ImGui::SameLine();
		ImGui::SetCursorPosX(RightAlignedX);
	}

	// 우측 버튼 1: ViewType (Base 클래스 함수 사용)
	RenderViewTypeButton(ViewportClient, ViewTypeLabels, RightViewTypeButtonWidthDefault);

	ImGui::SameLine(0.0f, RightButtonSpacing);

	// 우측 버튼 2: Camera Settings (Base 클래스 함수 사용)
	RenderCameraSpeedButton(Camera, CameraSpeedButtonWidth);

	// 카메라 설정 팝업
	if (ImGui::BeginPopup("##CameraSettingsPopup"))
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

		RenderCameraSettingsPopupContent();

		ImGui::PopStyleColor(4);
		ImGui::EndPopup();
	}

	ImGui::SameLine(0.0f, RightButtonSpacing);

	// 우측 버튼 3: ViewMode (Base 클래스 함수 사용)
	RenderViewModeButton(ViewportClient, ViewModeLabels);

	ImGui::PopStyleVar(3);
}

void USkeletalMeshViewerToolbarWidget::RenderCameraSettingsPopupContent()
{
	if (!ViewportClient)
	{
		return;
	}

	UCamera* Camera = ViewportClient->GetCamera();
	if (!Camera)
	{
		return;
	}

	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Camera Settings");
	ImGui::Separator();
	ImGui::Spacing();

	// 위치
	FVector Location = Camera->GetLocation();
	float Loc[3] = { Location.X, Location.Y, Location.Z };
	if (ImGui::DragFloat3("Location", Loc, 1.0f))
	{
		Camera->SetLocation(FVector(Loc[0], Loc[1], Loc[2]));
	}

	// 회전
	FVector Rotation = Camera->GetRotation();
	float Rot[3] = { Rotation.X, Rotation.Y, Rotation.Z };
	if (ImGui::DragFloat3("Rotation", Rot, 1.0f))
	{
		Camera->SetRotation(FVector(Rot[0], Rot[1], Rot[2]));
	}

	ImGui::Spacing();

	// Perspective 전용 설정
	ECameraType CameraType = Camera->GetCameraType();
	if (CameraType == ECameraType::ECT_Perspective)
	{
		float FOV = Camera->GetFovY();
		if (ImGui::DragFloat("FOV", &FOV, 0.1f, 10.0f, 120.0f, "%.1f"))
		{
			Camera->SetFovY(FOV);
		}
	}
	// Orthographic 전용 설정
	else
	{
		float OrthoZoom = Camera->GetOrthoZoom();
		if (ImGui::SliderFloat("Ortho Zoom", &OrthoZoom, 10.0f, 10000.0f, "%.1f"))
		{
			Camera->SetOrthoZoom(OrthoZoom);
		}
	}

	ImGui::Spacing();

	// Near/Far Z 클리핑 평면
	float NearZ = Camera->GetNearZ();
	if (ImGui::DragFloat("Near Z", &NearZ, 0.01f, 0.01f, 100.0f, "%.2f"))
	{
		Camera->SetNearZ(NearZ);
	}

	float FarZ = Camera->GetFarZ();
	if (ImGui::DragFloat("Far Z", &FarZ, 10.0f, 100.0f, 100000.0f, "%.1f"))
	{
		Camera->SetFarZ(FarZ);
	}

	ImGui::Spacing();

	// 카메라 이동 속도
	float MoveSpeed = Camera->GetMoveSpeed();
	if (ImGui::DragFloat("Move Speed", &MoveSpeed, 0.1f, 0.1f, 100.0f, "%.1f"))
	{
		Camera->SetMoveSpeed(MoveSpeed);
	}

	ImGui::Spacing();
	ImGui::Separator();

	// 카메라 리셋 버튼
	if (ImGui::Button("Reset Camera", ImVec2(-1, 0)))
	{
		Camera->SetLocation(FVector(0.0f, -5.0f, 4.0f));
		Camera->SetRotation(FVector(0.0f, 0.0f, 90.0f));
		Camera->SetFovY(90.0f);
		Camera->SetOrthoZoom(1000.0f);
		Camera->SetMoveSpeed(10.0f);
	}
}

