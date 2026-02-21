#include "pch.h"
#include "Render/UI/Widget/Public/ViewportControlWidget.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/EditorEngine.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Texture/Public/Texture.h"
#include "Actor/Public/Actor.h"

// 정적 멤버 정의 - KTLWeek07의 뷰 모드 기능 사용
const char* UViewportControlWidget::ViewModeLabels[] = {
	"Gouraud", "Lambert", "BlinnPhong", "Unlit", "Wireframe", "SceneDepth", "WorldNormal"
};

const char* UViewportControlWidget::ViewTypeLabels[] = {
	"Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back"
};

UViewportControlWidget::UViewportControlWidget()
	: UViewportToolbarWidgetBase()
{
	SetName("Viewport Control Widget");
}

UViewportControlWidget::~UViewportControlWidget() = default;

void UViewportControlWidget::Initialize()
{
	LoadCommonIcons();
	LoadViewportSpecificIcons();
	UE_LOG("ViewportControlWidget: Initialized");
}

void UViewportControlWidget::LoadViewportSpecificIcons()
{
	UE_LOG("ViewportControlWidget: 전용 아이콘 로드 시작...");
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	UPathManager& PathManager = UPathManager::GetInstance();
	FString IconBasePath = PathManager.GetAssetPath().string() + "\\Icon\\";

	int32 LoadedCount = 0;

	// 레이아웃 전환 아이콘 로드 (ViewportControl 전용)
	IconQuad = AssetManager.LoadTexture((IconBasePath + "quad.png").data());
	if (IconQuad) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'quad' -> %p", IconQuad);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "quad.png").c_str());
	}

	IconSquare = AssetManager.LoadTexture((IconBasePath + "square.png").data());
	if (IconSquare) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'square' -> %p", IconSquare);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "square.png").c_str());
	}

	UE_LOG_SUCCESS("ViewportControlWidget: 전용 아이콘 로드 완료 (%d/2)", LoadedCount);
}

void UViewportControlWidget::Update()
{
	// 필요시 업데이트 로직 추가

}

void UViewportControlWidget::RenderWidget()
{
	auto& ViewportManager = UViewportManager::GetInstance();
	if (!ViewportManager.GetRoot())
	{
		return;
	}

	// 먼저 스플리터 선 렌더링 (UI 뒤에서)
	//RenderSplitterLines();

	// 싱글 모드에서는 하나만 렌더링
	if (ViewportManager.GetViewportLayout() == EViewportLayout::Single)
	{
		int32 ActiveViewportIndex = ViewportManager.GetActiveIndex();
		RenderViewportToolbar(ActiveViewportIndex);
	}
	else
	{
		for (int32 i = 0;i < 4;++i)
		{
			RenderViewportToolbar(i);
		}
	}
}

void UViewportControlWidget::RenderViewportToolbar(int32 ViewportIndex)
{
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	const auto& Clients = ViewportManager.GetClients();

	// 뷰포트 범위가 벗어날 경우 종료
	if (ViewportIndex >= Viewports.Num() || ViewportIndex >= Clients.Num())
	{
		return;
	}

	// FutureEngine: null 체크 추가
	if (!Viewports[ViewportIndex] || !Clients[ViewportIndex])
	{
		return;
	}

	const FRect& Rect = Viewports[ViewportIndex]->GetRect();
	if (Rect.Width <= 0 || Rect.Height <= 0)
	{
		return;
	}


	constexpr int32 ToolbarH = 32;
	const ImVec2 Vec1{ static_cast<float>(Rect.Left), static_cast<float>(Rect.Top) };
	const ImVec2 Vec2{ static_cast<float>(Rect.Left + Rect.Width), static_cast<float>(Rect.Top + ToolbarH) };

	// 1) 툴바 배경 그리기
	ImDrawList* DrawLine = ImGui::GetBackgroundDrawList();
	DrawLine->AddRectFilled(Vec1, Vec2, IM_COL32(30, 30, 30, 100));
	DrawLine->AddLine(ImVec2(Vec1.x, Vec2.y), ImVec2(Vec2.x, Vec2.y), IM_COL32(70, 70, 70, 120), 1.0f);

	// 2) 툴바 UI 요소들을 직접 배치 (별도 창 생성하지 않음)
	// UI 요소들을 화면 좌표계에서 직접 배치
	ImGui::SetNextWindowPos(ImVec2(Vec1.x, Vec1.y));
	ImGui::SetNextWindowSize(ImVec2(Vec2.x - Vec1.x, Vec2.y - Vec1.y));

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoTitleBar;

	// 스타일 설정
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));

	char WinName[64];
	(void)snprintf(WinName, sizeof(WinName), "##ViewportToolbar%d", ViewportIndex);

	ImGui::PushID(ViewportIndex);
	if (ImGui::Begin(WinName, nullptr, flags))
	{
		// ========================================
		// PIE 상태 확인
		// ========================================
		const bool bIsPIEActive = (GEditor && GEditor->IsPIESessionActive());
		const bool bIsPIEMouseDetached = (GEditor && GEditor->IsPIEMouseDetached());
		const bool bIsPIEViewport = (bIsPIEActive && ViewportIndex == ViewportManager.GetPIEActiveViewportIndex());

		// ========================================
		// Gizmo Mode 버튼들 (Select/Translate/Rotate/Scale)
		// ========================================
		UEditor* Editor = GEditor ? GEditor->GetEditorModule() : nullptr;
		UGizmo* Gizmo = Editor ? Editor->GetGizmo() : nullptr;
		EGizmoMode CurrentGizmoMode = Gizmo ? Gizmo->GetGizmoMode() : EGizmoMode::Translate;

		constexpr float GizmoButtonSize = 24.0f;
		constexpr float GizmoIconSize = 16.0f;
		constexpr float GizmoButtonSpacing = 4.0f;

		// 기즈모 버튼 렌더링
		if (!bIsPIEViewport)
		{
			// Gizmo Mode 버튼들 (Base 클래스 함수 사용)
			EGizmoMode OutMode;
			bool OutSelect;
			RenderGizmoModeButtons(CurrentGizmoMode, false, OutMode, OutSelect); // Select 모드는 미지원
			if (Gizmo && OutMode != CurrentGizmoMode)
			{
				Gizmo->SetGizmoMode(OutMode);
			}

			// ========================================
			// World/Local Space Toggle
			// ========================================

			ImGui::SameLine(0.0f, 8.0f);

			// World/Local 토글 (Base 클래스 함수 사용)
			if (Gizmo)
			{
				RenderWorldLocalToggle(Gizmo);
			}

			// ========================================
			// Location Snap 버튼들
			// ========================================
			bool bLocationSnapEnabled = ViewportManager.IsLocationSnapEnabled();
			float LocationSnapValue = ViewportManager.GetLocationSnapValue();
			float OldLocationSnapValue = LocationSnapValue;

			// Base 클래스의 공통 렌더링 함수 사용
			RenderLocationSnapControls(bLocationSnapEnabled, LocationSnapValue);

			// 값이 변경되었으면 ViewportManager에 반영 및 Grid 동기화
			if (bLocationSnapEnabled != ViewportManager.IsLocationSnapEnabled())
			{
				ViewportManager.SetLocationSnapEnabled(bLocationSnapEnabled);
			}
			if (std::abs(LocationSnapValue - OldLocationSnapValue) > 0.01f)
			{
				ViewportManager.SetLocationSnapValue(LocationSnapValue);

				// Grid 크기 동기화
				if (Editor)
				{
					UBatchLines* BatchLine = Editor->GetBatchLines();
					if (BatchLine)
					{
						BatchLine->UpdateUGridVertices(LocationSnapValue);
					}
				}
			}
		}
		else
		{
			// PIE 상태 텍스트 표시
			if (!bIsPIEMouseDetached)
			{
				// Dummy 공간 확보
				ImGui::Dummy(ImVec2(0.0f, GizmoButtonSize));

				// DrawList로 직접 텍스트 렌더링
				ImDrawList* DL = ImGui::GetWindowDrawList();
				const float TextHeight = ImGui::GetTextLineHeight();
				const float ButtonCenterOffset = (GizmoButtonSize - TextHeight) * 0.5f;
				ImVec2 TextPos = ImGui::GetCursorScreenPos();
				TextPos.y = TextPos.y - GizmoButtonSize + ButtonCenterOffset;

				DL->AddText(TextPos, IM_COL32(200, 200, 100, 255), "Playing (Shift + F1 to detach mouse)");
			}
		}

		// ========================================
		// Rotation Snap 버튼들
		// ========================================
		if (!bIsPIEViewport)
		{
			bool bRotationSnapEnabled = ViewportManager.IsRotationSnapEnabled();
			float RotationSnapAngle = ViewportManager.GetRotationSnapAngle();

			// Base 클래스의 공통 렌더링 함수 사용
			RenderRotationSnapControls(bRotationSnapEnabled, RotationSnapAngle);

			// 값이 변경되었으면 ViewportManager에 반영
			if (bRotationSnapEnabled != ViewportManager.IsRotationSnapEnabled())
			{
				ViewportManager.SetRotationSnapEnabled(bRotationSnapEnabled);
			}
			if (std::abs(RotationSnapAngle - ViewportManager.GetRotationSnapAngle()) > 0.01f)
			{
				ViewportManager.SetRotationSnapAngle(RotationSnapAngle);
			}
		}

		// ========================================
		// Scale Snap 버튼들
		// ========================================
		if (!bIsPIEViewport)
		{
			bool bScaleSnapEnabled = ViewportManager.IsScaleSnapEnabled();
			float ScaleSnapValue = ViewportManager.GetScaleSnapValue();

			// Base 클래스의 공통 렌더링 함수 사용
			RenderScaleSnapControls(bScaleSnapEnabled, ScaleSnapValue);

			// 값이 변경되었으면 ViewportManager에 반영
			if (bScaleSnapEnabled != ViewportManager.IsScaleSnapEnabled())
			{
				ViewportManager.SetScaleSnapEnabled(bScaleSnapEnabled);
			}
			if (std::abs(ScaleSnapValue - ViewportManager.GetScaleSnapValue()) > 0.0001f)
			{
				ViewportManager.SetScaleSnapValue(ScaleSnapValue);
			}
		}

		// 우측 정렬할 버튼들의 총 너비 계산
		bool bInPilotMode = IsPilotModeActive(ViewportIndex);
		AActor* PilotedActor = (Editor && bInPilotMode) ? Editor->GetPilotedActor() : nullptr;

		// ViewType 버튼 폭 계산
		constexpr float RightViewTypeButtonWidthDefault = 110.0f;
		constexpr float RightViewTypeIconSize = 16.0f;
		constexpr float RightViewTypePadding = 4.0f;
		float RightViewTypeButtonWidth = RightViewTypeButtonWidthDefault;

		// Actor 이름 저장용
		static FString CachedActorName;

		if (bInPilotMode && PilotedActor)
		{
			CachedActorName = PilotedActor->GetName().ToString();
			const ImVec2 ActorNameTextSize = ImGui::CalcTextSize(CachedActorName.c_str());
			// 아이콘 + 패딩 + 텍스트 + 패딩
			RightViewTypeButtonWidth = RightViewTypePadding + RightViewTypeIconSize + RightViewTypePadding + ActorNameTextSize.x + RightViewTypePadding;
			// 최소 / 최대 폭 제한
			RightViewTypeButtonWidth = Clamp(RightViewTypeButtonWidth, 110.0f, 250.0f);
		}

		// ViewMode 버튼 폭 계산
		EViewModeIndex CurrentMode = Clients[ViewportIndex]->GetViewMode();
		int32 CurrentModeIndex = static_cast<int32>(CurrentMode);
		constexpr float ViewModeButtonHeight = 24.0f;
		constexpr float ViewModeIconSize = 16.0f;
		constexpr float ViewModePadding = 4.0f;
		const ImVec2 ViewModeTextSize = ImGui::CalcTextSize(ViewModeLabels[CurrentModeIndex]);
		const float ViewModeButtonWidth = ViewModePadding + ViewModeIconSize + ViewModePadding + ViewModeTextSize.x + ViewModePadding;

		constexpr float CameraSpeedButtonWidth = 70.0f; // 아이콘 + 숫자 표시를 위해 확장
		constexpr float LayoutToggleButtonSize = 24.0f;
		constexpr float RightButtonSpacing = 6.0f;
		constexpr float PilotExitButtonSize = 24.0f;
		const float PilotModeExtraWidth = bInPilotMode ? (PilotExitButtonSize + RightButtonSpacing) : 0.0f;
		const float TotalRightButtonsWidth = RightViewTypeButtonWidth + RightButtonSpacing + PilotModeExtraWidth + CameraSpeedButtonWidth + RightButtonSpacing + ViewModeButtonWidth + RightButtonSpacing + LayoutToggleButtonSize;

		// 우측 정렬 시작
		{
			const float ContentRegionRight = ImGui::GetWindowContentRegionMax().x;
			float RightAlignedX = ContentRegionRight - TotalRightButtonsWidth - 6.0f;
			RightAlignedX = std::max(RightAlignedX, ImGui::GetCursorPosX());

			ImGui::SameLine();
			ImGui::SetCursorPosX(RightAlignedX);
		}

		// 우측 버튼 1: 카메라 뷰 타입 (Base 클래스 함수 사용, 파일럿 모드 지원)
		{
			// 파일럿 모드 정보 준비
			const char* ActorName = (bInPilotMode && !CachedActorName.empty()) ? CachedActorName.c_str() : "";

			RenderViewTypeButton(Clients[ViewportIndex], ViewTypeLabels, RightViewTypeButtonWidth, bInPilotMode, ActorName);

			// ViewType 변경 시 파일럿 모드 종료 로직은 베이스 클래스에서 처리 못하므로 별도 처리 필요
			// 하지만 베이스 클래스에서 이미 ViewType 변경을 처리하므로, 여기서는 파일럿 모드 종료만 확인
			if (ImGui::BeginPopup("##ViewTypeDropdown"))
			{
				// 베이스 클래스가 이미 팝업을 열었으므로, 여기서는 파일럿 모드 종료 로직만 추가
				// 실제로는 베이스 클래스 팝업이 처리하므로 여기는 도달하지 않음
				ImGui::EndPopup();
			}
		}

		ImGui::SameLine(0.0f, RightButtonSpacing);

		// 파일럿 모드 Exit 버튼 (ViewType 버튼 옆)
		if (IsPilotModeActive(ViewportIndex))
		{
			ImVec2 ExitButtonCursorPos = ImGui::GetCursorScreenPos();
			RenderPilotModeExitButton(ViewportIndex, ExitButtonCursorPos);
			ImGui::SetCursorScreenPos(ExitButtonCursorPos);
			ImGui::SameLine(0.0f, RightButtonSpacing);
		}

		// 우측 버튼 2: 카메라 속도 (Base 클래스 함수 사용)
		if (UCamera* Camera = Clients[ViewportIndex]->GetCamera())
		{
			RenderCameraSpeedButton(Camera, CameraSpeedButtonWidth);

			// 카메라 설정 팝업
			if (ImGui::BeginPopup("##CameraSettingsPopup"))
			{
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

				if (UCamera* Camera = Clients[ViewportIndex]->GetCamera())
				{
					EViewType PopupViewType = Clients[ViewportIndex]->GetViewType();
					const bool bIsPerspective = (PopupViewType == EViewType::Perspective);
					ImGui::Text(bIsPerspective ? "Perspective Camera Settings" : "Orthographic Camera Settings");
					ImGui::Separator();

					// 전역 카메라 이동 속도
					float editorCameraSpeed = ViewportManager.GetEditorCameraSpeed();
					if (ImGui::DragFloat("Camera Speed", &editorCameraSpeed, 0.1f,
						UViewportManager::MIN_CAMERA_SPEED, UViewportManager::MAX_CAMERA_SPEED, "%.1f"))
					{
						ViewportManager.SetEditorCameraSpeed(editorCameraSpeed);
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Applies to all viewports");
					}

					// 카메라 위치
					FVector location = Camera->GetLocation();
					if (ImGui::DragFloat3("Location", &location.X, 0.1f))
					{
						Camera->SetLocation(location);
					}

					// 카메라 회전 (Perspective만 표시)
					if (bIsPerspective)
					{
						static FVector CachedRotation = FVector::ZeroVector();
						static bool bIsDraggingRotation = false;

						// 드래그 시작 시 또는 비활성 상태일 때 현재 값 캐싱
						if (!bIsDraggingRotation)
						{
							CachedRotation = Camera->GetRotation();
						}

						bool bRotationChanged = ImGui::DragFloat3("Rotation", &CachedRotation.X, 0.5f);

						// 드래그 상태 추적
						if (ImGui::IsItemActive())
						{
							bIsDraggingRotation = true;

							// 값이 변경되었으면 카메라에 반영
							if (bRotationChanged)
							{
								Camera->SetRotation(CachedRotation);
								// SetRotation 후 wrapping된 값으로 즉시 재동기화
								CachedRotation = Camera->GetRotation();
							}
						}
						else if (bIsDraggingRotation)
						{
							// 드래그 종료 시 최종 동기화
							bIsDraggingRotation = false;
							CachedRotation = Camera->GetRotation();
						}
					}
					// Orthographic 뷰는 회전 항목 없음 (고정된 방향)

					ImGui::Separator();

					if (bIsPerspective)
					{
						// Perspective: FOV 표시
						float Fov = Camera->GetFovY();
						if (ImGui::DragFloat("FOV", &Fov, 0.1f, 1.0f, 170.0f, "%.1f"))
						{
							Camera->SetFovY(Fov);
						}
					}
					else
					{
						// Orthographic: Zoom Level (OrthoZoom) 표시 및 SharedOrthoZoom 동기화
						float OrthoZoom = Camera->GetOrthoZoom();
						if (ImGui::DragFloat("Zoom Level", &OrthoZoom, 10.0f, 10.0f, 10000.0f, "%.1f"))
						{
							Camera->SetOrthoZoom(OrthoZoom);

							// 모든 Ortho 카메라에 동일한 줌 적용 (SharedOrthoZoom 갱신)
							for (FViewportClient* OtherClient : ViewportManager.GetClients())
							{
								if (OtherClient && OtherClient->IsOrtho())
								{
									if (UCamera* OtherCam = OtherClient->GetCamera())
									{
										OtherCam->SetOrthoZoom(OrthoZoom);
									}
								}
							}
						}

						// 정보: Aspect는 자동 계산됨
						float Aspect = Camera->GetAspect();
						ImGui::BeginDisabled();
						ImGui::DragFloat("Aspect Ratio", &Aspect, 0.01f, 0.1f, 10.0f, "%.3f");
						ImGui::EndDisabled();
					}

					// Near/Far Plane
					float NearZ = Camera->GetNearZ();
					if (ImGui::DragFloat("Near Z", &NearZ, 0.01f, 0.01f, 100.0f, "%.3f"))
					{
						Camera->SetNearZ(NearZ);
					}

					float FarZ = Camera->GetFarZ();
					if (ImGui::DragFloat("Far Z", &FarZ, 1.0f, 1.0f, 10000.0f, "%.1f"))
					{
						Camera->SetFarZ(FarZ);
					}
				}

				ImGui::PopStyleColor(4);
				ImGui::EndPopup();
			}
		}

		ImGui::SameLine(0.0f, RightButtonSpacing);

		// 우측 버튼 3: ViewMode 버튼 (Base 클래스 함수 사용)
		RenderViewModeButton(Clients[ViewportIndex], ViewModeLabels);

		ImGui::SameLine(0.0f, RightButtonSpacing);

		// 우측 버튼 4: 레이아웃 토글 버튼
		constexpr float LayoutToggleIconSize = 16.0f;

		if (ViewportManager.GetViewportLayout() == EViewportLayout::Single)
		{
			// Quad 레이아웃으로 전환 버튼
			if (IconQuad && IconQuad->GetTextureSRV())
			{
				const ImVec2 LayoutToggleQuadButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##LayoutToggleQuadButton", ImVec2(LayoutToggleButtonSize, LayoutToggleButtonSize));
				const bool bLayoutToggleQuadClicked = ImGui::IsItemClicked();
				const bool bLayoutToggleQuadHovered = ImGui::IsItemHovered();

				// 버튼 배경 그리기
				ImDrawList* LayoutToggleQuadDrawList = ImGui::GetWindowDrawList();
				ImU32 LayoutToggleQuadBgColor = bLayoutToggleQuadHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
				if (ImGui::IsItemActive())
				{
					LayoutToggleQuadBgColor = IM_COL32(38, 38, 38, 255);
				}
				LayoutToggleQuadDrawList->AddRectFilled(LayoutToggleQuadButtonPos, ImVec2(LayoutToggleQuadButtonPos.x + LayoutToggleButtonSize, LayoutToggleQuadButtonPos.y + LayoutToggleButtonSize), LayoutToggleQuadBgColor, 4.0f);
				LayoutToggleQuadDrawList->AddRect(LayoutToggleQuadButtonPos, ImVec2(LayoutToggleQuadButtonPos.x + LayoutToggleButtonSize, LayoutToggleQuadButtonPos.y + LayoutToggleButtonSize), IM_COL32(96, 96, 96, 255), 4.0f);

				// 아이콘 그리기 (중앙 정렬)
				const ImVec2 LayoutToggleQuadIconPos = ImVec2(
					LayoutToggleQuadButtonPos.x + (LayoutToggleButtonSize - LayoutToggleIconSize) * 0.5f,
					LayoutToggleQuadButtonPos.y + (LayoutToggleButtonSize - LayoutToggleIconSize) * 0.5f
				);
				LayoutToggleQuadDrawList->AddImage(
					IconQuad->GetTextureSRV(),
					LayoutToggleQuadIconPos,
					ImVec2(LayoutToggleQuadIconPos.x + LayoutToggleIconSize, LayoutToggleQuadIconPos.y + LayoutToggleIconSize)
				);

				if (bLayoutToggleQuadClicked)
				{
					CurrentLayout = ELayout::Quad;
					ViewportManager.SetViewportLayout(EViewportLayout::Quad);
					ViewportManager.StartLayoutAnimation(true, ViewportIndex);
				}
			}
		}

		if (ViewportManager.GetViewportLayout() == EViewportLayout::Quad)
		{
			// Single 레이아웃으로 전환 버튼
			if (IconSquare && IconSquare->GetTextureSRV())
			{
				const ImVec2 LayoutToggleSingleButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##LayoutToggleSingleButton", ImVec2(LayoutToggleButtonSize, LayoutToggleButtonSize));
				const bool bLayoutToggleSingleClicked = ImGui::IsItemClicked();
				const bool bLayoutToggleSingleHovered = ImGui::IsItemHovered();

				// 버튼 배경 그리기
				ImDrawList* LayoutToggleSingleDrawList = ImGui::GetWindowDrawList();
				ImU32 LayoutToggleSingleBgColor = bLayoutToggleSingleHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
				if (ImGui::IsItemActive())
				{
					LayoutToggleSingleBgColor = IM_COL32(38, 38, 38, 255);
				}
				LayoutToggleSingleDrawList->AddRectFilled(LayoutToggleSingleButtonPos, ImVec2(LayoutToggleSingleButtonPos.x + LayoutToggleButtonSize, LayoutToggleSingleButtonPos.y + LayoutToggleButtonSize), LayoutToggleSingleBgColor, 4.0f);
				LayoutToggleSingleDrawList->AddRect(LayoutToggleSingleButtonPos, ImVec2(LayoutToggleSingleButtonPos.x + LayoutToggleButtonSize, LayoutToggleSingleButtonPos.y + LayoutToggleButtonSize), IM_COL32(96, 96, 96, 255), 4.0f);

				// 아이콘 그리기 (중앙 정렬)
				const ImVec2 LayoutToggleSingleIconPos = ImVec2(
					LayoutToggleSingleButtonPos.x + (LayoutToggleButtonSize - LayoutToggleIconSize) * 0.5f,
					LayoutToggleSingleButtonPos.y + (LayoutToggleButtonSize - LayoutToggleIconSize) * 0.5f
				);
				LayoutToggleSingleDrawList->AddImage(
					IconSquare->GetTextureSRV(),
					LayoutToggleSingleIconPos,
					ImVec2(LayoutToggleSingleIconPos.x + LayoutToggleIconSize, LayoutToggleSingleIconPos.y + LayoutToggleIconSize)
				);

				if (bLayoutToggleSingleClicked)
				{
					CurrentLayout = ELayout::Single;
					ViewportManager.SetViewportLayout(EViewportLayout::Single);
					// 스플리터 비율을 저장하고 애니메이션 시작: Quad → Single
					ViewportManager.PersistSplitterRatios();
					ViewportManager.StartLayoutAnimation(false, ViewportIndex);
				}
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(3);
	ImGui::PopID();
}

bool UViewportControlWidget::IsPilotModeActive(int32 ViewportIndex) const
{
	if (!GEditor)
	{
		return false;
	}

	UEditor* Editor = GEditor->GetEditorModule();
	if (!Editor || !Editor->IsPilotMode())
	{
		return false;
	}

	// 현재 뷰포트가 파일럿 모드를 사용 중인지 확인
	auto& ViewportManager = UViewportManager::GetInstance();
	return (ViewportManager.GetLastClickedViewportIndex() == ViewportIndex);
}

void UViewportControlWidget::RenderPilotModeExitButton(int32 ViewportIndex, ImVec2& InOutCursorPos)
{
	if (!GEditor)
	{
		return;
	}

	UEditor* Editor = GEditor->GetEditorModule();
	if (!Editor || !Editor->IsPilotMode())
	{
		return;
	}

	// △ 아이콘 버튼 (파일럿 모드 종료)
	constexpr float ExitButtonSize = 24.0f;
	constexpr float TriangleSize = 8.0f;

	ImVec2 ExitButtonPos = InOutCursorPos;
	ImGui::SetCursorScreenPos(ExitButtonPos);
	ImGui::InvisibleButton("##PilotModeExit", ImVec2(ExitButtonSize, ExitButtonSize));
	bool bExitClicked = ImGui::IsItemClicked();
	bool bExitHovered = ImGui::IsItemHovered();

	// 버튼 배경
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImU32 BgColor = bExitHovered ? IM_COL32(60, 40, 40, 255) : IM_COL32(40, 20, 20, 255);
	if (ImGui::IsItemActive())
	{
		BgColor = IM_COL32(80, 50, 50, 255);
	}
	DrawList->AddRectFilled(ExitButtonPos, ImVec2(ExitButtonPos.x + ExitButtonSize, ExitButtonPos.y + ExitButtonSize), BgColor, 4.0f);
	DrawList->AddRect(ExitButtonPos, ImVec2(ExitButtonPos.x + ExitButtonSize, ExitButtonPos.y + ExitButtonSize), IM_COL32(160, 80, 80, 255), 4.0f);

	// △ 아이콘 그리기 (Eject 느낌)
	ImVec2 Center = ImVec2(ExitButtonPos.x + ExitButtonSize * 0.5f, ExitButtonPos.y + ExitButtonSize * 0.5f);
	ImVec2 P1 = ImVec2(Center.x, Center.y - TriangleSize * 0.6f);
	ImVec2 P2 = ImVec2(Center.x - TriangleSize * 0.5f, Center.y + TriangleSize * 0.4f);
	ImVec2 P3 = ImVec2(Center.x + TriangleSize * 0.5f, Center.y + TriangleSize * 0.4f);
	DrawList->AddTriangleFilled(P1, P2, P3, IM_COL32(220, 180, 180, 255));

	if (bExitClicked)
	{
		Editor->RequestExitPilotMode();
		UE_LOG_INFO("ViewportControlWidget: Pilot mode exited via UI button");
	}

	if (bExitHovered)
	{
		ImGui::SetTooltip("Exit Pilot Mode (Alt + G)");
	}

	// 커서 위치 업데이트 (다음 버튼 배치용)
	InOutCursorPos.x += ExitButtonSize + 6.0f;
}
