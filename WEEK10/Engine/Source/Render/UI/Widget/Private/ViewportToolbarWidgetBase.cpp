#include "pch.h"
#include "Render/UI/Widget/Public/ViewportToolbarWidgetBase.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Texture/Public/Texture.h"
#include "ImGui/imgui_internal.h"

UViewportToolbarWidgetBase::UViewportToolbarWidgetBase()
	: UWidget("Viewport Toolbar Widget Base")
{
}

UViewportToolbarWidgetBase::~UViewportToolbarWidgetBase() = default;

void UViewportToolbarWidgetBase::LoadCommonIcons()
{
	if (bIconsLoaded)
	{
		return;
	}

	UE_LOG("ViewportToolbarWidgetBase: 공통 아이콘 로드 시작...");
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	UPathManager& PathManager = UPathManager::GetInstance();
	FString IconBasePath = PathManager.GetAssetPath().string() + "\\Icon\\";

	int32 LoadedCount = 0;

	// ViewType 아이콘 로드
	IconPerspective = AssetManager.LoadTexture((IconBasePath + "ViewPerspective.png").data());
	if (IconPerspective) { ++LoadedCount; }

	IconTop = AssetManager.LoadTexture((IconBasePath + "ViewTop.png").data());
	if (IconTop) { ++LoadedCount; }

	IconBottom = AssetManager.LoadTexture((IconBasePath + "ViewBottom.png").data());
	if (IconBottom) { ++LoadedCount; }

	IconLeft = AssetManager.LoadTexture((IconBasePath + "ViewLeft.png").data());
	if (IconLeft) { ++LoadedCount; }

	IconRight = AssetManager.LoadTexture((IconBasePath + "ViewRight.png").data());
	if (IconRight) { ++LoadedCount; }

	IconFront = AssetManager.LoadTexture((IconBasePath + "ViewFront.png").data());
	if (IconFront) { ++LoadedCount; }

	IconBack = AssetManager.LoadTexture((IconBasePath + "ViewBack.png").data());
	if (IconBack) { ++LoadedCount; }

	// Gizmo Mode 아이콘 로드
	IconSelect = AssetManager.LoadTexture((IconBasePath + "Select.png").data());
	if (IconSelect) { ++LoadedCount; }

	IconTranslate = AssetManager.LoadTexture((IconBasePath + "Translate.png").data());
	if (IconTranslate) { ++LoadedCount; }

	IconRotate = AssetManager.LoadTexture((IconBasePath + "Rotate.png").data());
	if (IconRotate) { ++LoadedCount; }

	IconScale = AssetManager.LoadTexture((IconBasePath + "Scale.png").data());
	if (IconScale) { ++LoadedCount; }

	// World/Local Space 아이콘 로드
	IconWorldSpace = AssetManager.LoadTexture((IconBasePath + "WorldSpace.png").data());
	if (IconWorldSpace) { ++LoadedCount; }

	IconLocalSpace = AssetManager.LoadTexture((IconBasePath + "LocalSpace.png").data());
	if (IconLocalSpace) { ++LoadedCount; }

	// Snap 아이콘 로드
	IconSnapLocation = AssetManager.LoadTexture((IconBasePath + "SnapLocation.png").data());
	if (IconSnapLocation) { ++LoadedCount; }

	IconSnapRotation = AssetManager.LoadTexture((IconBasePath + "SnapRotation.png").data());
	if (IconSnapRotation) { ++LoadedCount; }

	IconSnapScale = AssetManager.LoadTexture((IconBasePath + "SnapScale.png").data());
	if (IconSnapScale) { ++LoadedCount; }

	// ViewMode 아이콘 로드
	IconLitCube = AssetManager.LoadTexture((IconBasePath + "LitCube.png").data());
	if (IconLitCube) { ++LoadedCount; }

	// 카메라 설정 아이콘 로드
	IconCamera = AssetManager.LoadTexture((IconBasePath + "Camera.png").data());
	if (IconCamera) { ++LoadedCount; }

	UE_LOG_SUCCESS("ViewportToolbarWidgetBase: 공통 아이콘 로드 완료 (%d/18)", LoadedCount);
	bIconsLoaded = true;
}

bool UViewportToolbarWidgetBase::DrawIconButton(const char* ID, UTexture* Icon, bool bActive, const char* Tooltip, float ButtonSize, float IconSize)
{
	if (!Icon || !Icon->GetTextureSRV())
	{
		return false;
	}

	ImVec2 ButtonPos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton(ID, ImVec2(ButtonSize, ButtonSize));
	bool bClicked = ImGui::IsItemClicked();
	bool bHovered = ImGui::IsItemHovered();

	ImDrawList* DL = ImGui::GetWindowDrawList();

	// 배경색
	ImU32 BgColor = bActive ? IM_COL32(20, 20, 20, 255) : (bHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255));
	if (ImGui::IsItemActive()) BgColor = IM_COL32(38, 38, 38, 255);

	// 배경 렌더링
	DL->AddRectFilled(ButtonPos, ImVec2(ButtonPos.x + ButtonSize, ButtonPos.y + ButtonSize), BgColor, 4.0f);

	// 테두리
	ImU32 BorderColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(96, 96, 96, 255);
	DL->AddRect(ButtonPos, ImVec2(ButtonPos.x + ButtonSize, ButtonPos.y + ButtonSize), BorderColor, 4.0f);

	// 아이콘 (활성화 시 파란색 틴트)
	ImVec2 IconPos = ImVec2(ButtonPos.x + (ButtonSize - IconSize) * 0.5f, ButtonPos.y + (ButtonSize - IconSize) * 0.5f);
	ImU32 TintColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(255, 255, 255, 255);
	DL->AddImage(Icon->GetTextureSRV(), IconPos, ImVec2(IconPos.x + IconSize, IconPos.y + IconSize), ImVec2(0, 0), ImVec2(1, 1), TintColor);

	// 툴팁
	if (bHovered && Tooltip)
	{
		ImGui::SetTooltip("%s", Tooltip);
	}

	return bClicked;
}

void UViewportToolbarWidgetBase::RenderGizmoModeButtons(EGizmoMode CurrentGizmoMode, bool bSelectModeActive, EGizmoMode& OutGizmoMode, bool& OutSelectMode)
{
	constexpr float GizmoButtonSize = 24.0f;
	constexpr float GizmoIconSize = 16.0f;
	constexpr float GizmoButtonSpacing = 4.0f;

	OutGizmoMode = CurrentGizmoMode;
	OutSelectMode = bSelectModeActive;

	// Select 버튼 (Q)
	if (DrawIconButton("##GizmoSelect", IconSelect, bSelectModeActive, "Select (Q)", GizmoButtonSize, GizmoIconSize))
	{
		OutSelectMode = true;
	}

	ImGui::SameLine(0.0f, GizmoButtonSpacing);

	// Translate 버튼 (W)
	bool bTranslateActive = (CurrentGizmoMode == EGizmoMode::Translate && !bSelectModeActive);
	if (DrawIconButton("##GizmoTranslate", IconTranslate, bTranslateActive, "Translate (W)", GizmoButtonSize, GizmoIconSize))
	{
		OutGizmoMode = EGizmoMode::Translate;
		OutSelectMode = false;
	}

	ImGui::SameLine(0.0f, GizmoButtonSpacing);

	// Rotate 버튼 (E)
	bool bRotateActive = (CurrentGizmoMode == EGizmoMode::Rotate && !bSelectModeActive);
	if (DrawIconButton("##GizmoRotate", IconRotate, bRotateActive, "Rotate (E)", GizmoButtonSize, GizmoIconSize))
	{
		OutGizmoMode = EGizmoMode::Rotate;
		OutSelectMode = false;
	}

	ImGui::SameLine(0.0f, GizmoButtonSpacing);

	// Scale 버튼 (R)
	bool bScaleActive = (CurrentGizmoMode == EGizmoMode::Scale && !bSelectModeActive);
	if (DrawIconButton("##GizmoScale", IconScale, bScaleActive, "Scale (R)", GizmoButtonSize, GizmoIconSize))
	{
		OutGizmoMode = EGizmoMode::Scale;
		OutSelectMode = false;
	}
}

void UViewportToolbarWidgetBase::RenderWorldLocalToggle(UGizmo* Gizmo)
{
	if (!Gizmo || !IconWorldSpace || !IconLocalSpace)
	{
		return;
	}

	constexpr float GizmoButtonSize = 24.0f;
	constexpr float GizmoIconSize = 16.0f;

	bool bIsWorldMode = Gizmo->IsWorldMode();
	UTexture* CurrentSpaceIcon = bIsWorldMode ? IconWorldSpace : IconLocalSpace;
	const char* Tooltip = bIsWorldMode ? "월드 스페이스 좌표 \n좌표계를 순환하려면 클릭하거나 Ctrl+`을 누르세요"
		: "로컬 스페이스 좌표 \n좌표계를 순환하려면 클릭하거나 Ctrl+`을 누르세요";

	if (DrawIconButton("##WorldLocalToggle", CurrentSpaceIcon, false, Tooltip, GizmoButtonSize, GizmoIconSize))
	{
		// World ↔ Local 토글
		if (bIsWorldMode)
		{
			Gizmo->SetLocal();
		}
		else
		{
			Gizmo->SetWorld();
		}
	}
}

void UViewportToolbarWidgetBase::RenderLocationSnapControls(bool& bSnapEnabled, float& SnapValue)
{
	ImGui::SameLine(0.0f, 8.0f);

	// 위치 스냅 토글 버튼
	if (IconSnapLocation && IconSnapLocation->GetTextureSRV())
	{
		constexpr float SnapToggleButtonSize = 24.0f;
		constexpr float SnapToggleIconSize = 16.0f;

		ImVec2 SnapToggleButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##LocationSnapToggle", ImVec2(SnapToggleButtonSize, SnapToggleButtonSize));
		bool bSnapToggleClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapToggleHovered = ImGui::IsItemHovered();

		ImDrawList* SnapToggleDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapToggleBgColor;
		if (bSnapEnabled)
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(40, 40, 40, 255) : IM_COL32(20, 20, 20, 255);
		}
		else
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		}
		if (ImGui::IsItemActive())
		{
			SnapToggleBgColor = IM_COL32(50, 50, 50, 255);
		}
		SnapToggleDrawList->AddRectFilled(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBgColor, 4.0f);

		// 테두리
		ImU32 SnapToggleBorderColor = bSnapEnabled ? IM_COL32(150, 150, 150, 255) : IM_COL32(96, 96, 96, 255);
		SnapToggleDrawList->AddRect(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBorderColor, 4.0f);

		// 아이콘 렌더링 (중앙 정렬)
		ImVec2 IconMin = ImVec2(
			SnapToggleButtonPos.x + (SnapToggleButtonSize - SnapToggleIconSize) * 0.5f,
			SnapToggleButtonPos.y + (SnapToggleButtonSize - SnapToggleIconSize) * 0.5f
		);
		ImVec2 IconMax = ImVec2(IconMin.x + SnapToggleIconSize, IconMin.y + SnapToggleIconSize);
		ImU32 IconTintColor = bSnapEnabled ? IM_COL32(46, 163, 255, 255) : IM_COL32(220, 220, 220, 255);
		SnapToggleDrawList->AddImage((void*)IconSnapLocation->GetTextureSRV(), IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), IconTintColor);

		if (bSnapToggleClicked)
		{
			bSnapEnabled = !bSnapEnabled;
		}

		if (bSnapToggleHovered)
		{
			ImGui::SetTooltip("Toggle location snap");
		}
	}

	ImGui::SameLine(0.0f, 4.0f);

	// 위치 스냅 값 선택 버튼
	{
		char SnapValueText[16];
		// Format with commas for readability (e.g., "10,000" instead of "1e+04")
		if (SnapValue >= 1000.0f)
		{
			(void)snprintf(SnapValueText, sizeof(SnapValueText), "%.0f", SnapValue);
			// Add commas manually
			std::string FormattedText = SnapValueText;
			int InsertPosition = FormattedText.length() - 3;
			while (InsertPosition > 0)
			{
				FormattedText.insert(InsertPosition, ",");
				InsertPosition -= 3;
			}
			strncpy_s(SnapValueText, FormattedText.c_str(), sizeof(SnapValueText) - 1);
		}
		else
		{
			(void)snprintf(SnapValueText, sizeof(SnapValueText), "%.0f", SnapValue);
		}

		constexpr float SnapValueButtonHeight = 24.0f;
		constexpr float SnapValuePadding = 8.0f;
		const ImVec2 SnapValueTextSize = ImGui::CalcTextSize(SnapValueText);
		const float SnapValueButtonWidth = SnapValueTextSize.x + SnapValuePadding * 2;

		ImVec2 SnapValueButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##LocationSnapValue", ImVec2(SnapValueButtonWidth, SnapValueButtonHeight));
		bool bSnapValueClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapValueHovered = ImGui::IsItemHovered();

		// 버튼 배경 그리기
		ImDrawList* SnapValueDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapValueBgColor = bSnapValueHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			SnapValueBgColor = IM_COL32(38, 38, 38, 255);
		}
		SnapValueDrawList->AddRectFilled(SnapValueButtonPos, ImVec2(SnapValueButtonPos.x + SnapValueButtonWidth, SnapValueButtonPos.y + SnapValueButtonHeight), SnapValueBgColor, 4.0f);
		SnapValueDrawList->AddRect(SnapValueButtonPos, ImVec2(SnapValueButtonPos.x + SnapValueButtonWidth, SnapValueButtonPos.y + SnapValueButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// 텍스트 그리기
		ImVec2 SnapValueTextPos = ImVec2(
			SnapValueButtonPos.x + SnapValuePadding,
			SnapValueButtonPos.y + (SnapValueButtonHeight - ImGui::GetTextLineHeight()) * 0.5f
		);
		SnapValueDrawList->AddText(SnapValueTextPos, IM_COL32(220, 220, 220, 255), SnapValueText);

		if (bSnapValueClicked)
		{
			ImGui::OpenPopup("##LocationSnapValuePopup");
		}

		// 값 선택 팝업
		if (ImGui::BeginPopup("##LocationSnapValuePopup"))
		{
			ImGui::Text("Location Snap Value");
			ImGui::Separator();

			constexpr float SnapValues[] = { 10000.0f, 5000.0f, 1000.0f, 500.0f, 100.0f, 50.0f, 10.0f, 5.0f, 1.0f };
			constexpr const char* SnapValueLabels[] = { "10,000", "5,000", "1,000", "500", "100", "50", "10", "5", "1" };

			for (int i = 0; i < IM_ARRAYSIZE(SnapValues); ++i)
			{
				const bool bIsSelected = (std::abs(SnapValue - SnapValues[i]) < 0.01f);
				if (ImGui::MenuItem(SnapValueLabels[i], nullptr, bIsSelected))
				{
					SnapValue = SnapValues[i];
				}
			}

			ImGui::EndPopup();
		}

		if (bSnapValueHovered)
		{
			ImGui::SetTooltip("Choose location snap value");
		}
	}
}

void UViewportToolbarWidgetBase::RenderRotationSnapControls(bool& bSnapEnabled, float& SnapAngle)
{
	ImGui::SameLine(0.0f, 8.0f);

	// 회전 스냅 토글 버튼
	if (IconSnapRotation && IconSnapRotation->GetTextureSRV())
	{
		constexpr float SnapToggleButtonSize = 24.0f;
		constexpr float SnapToggleIconSize = 16.0f;

		ImVec2 SnapToggleButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##RotationSnapToggle", ImVec2(SnapToggleButtonSize, SnapToggleButtonSize));
		bool bSnapToggleClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapToggleHovered = ImGui::IsItemHovered();

		ImDrawList* SnapToggleDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapToggleBgColor;
		if (bSnapEnabled)
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(40, 40, 40, 255) : IM_COL32(20, 20, 20, 255);
		}
		else
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		}
		if (ImGui::IsItemActive())
		{
			SnapToggleBgColor = IM_COL32(50, 50, 50, 255);
		}
		SnapToggleDrawList->AddRectFilled(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBgColor, 4.0f);

		// 테두리
		ImU32 SnapToggleBorderColor = bSnapEnabled ? IM_COL32(150, 150, 150, 255) : IM_COL32(96, 96, 96, 255);
		SnapToggleDrawList->AddRect(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBorderColor, 4.0f);

		// 아이콘 렌더링 (중앙 정렬)
		ImVec2 IconMin = ImVec2(
			SnapToggleButtonPos.x + (SnapToggleButtonSize - SnapToggleIconSize) * 0.5f,
			SnapToggleButtonPos.y + (SnapToggleButtonSize - SnapToggleIconSize) * 0.5f
		);
		ImVec2 IconMax = ImVec2(IconMin.x + SnapToggleIconSize, IconMin.y + SnapToggleIconSize);
		ImU32 IconTintColor = bSnapEnabled ? IM_COL32(46, 163, 255, 255) : IM_COL32(220, 220, 220, 255);
		SnapToggleDrawList->AddImage((void*)IconSnapRotation->GetTextureSRV(), IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), IconTintColor);

		if (bSnapToggleClicked)
		{
			bSnapEnabled = !bSnapEnabled;
		}

		if (bSnapToggleHovered)
		{
			ImGui::SetTooltip("Toggle rotation snap");
		}
	}

	ImGui::SameLine(0.0f, 4.0f);

	// 회전 스냅 각도 선택 버튼
	{
		char SnapAngleText[16];
		(void)snprintf(SnapAngleText, sizeof(SnapAngleText), "%.0f°", SnapAngle);

		constexpr float SnapAngleButtonHeight = 24.0f;
		constexpr float SnapAnglePadding = 8.0f;
		const ImVec2 SnapAngleTextSize = ImGui::CalcTextSize(SnapAngleText);
		const float SnapAngleButtonWidth = SnapAngleTextSize.x + SnapAnglePadding * 2;

		ImVec2 SnapAngleButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##RotationSnapAngle", ImVec2(SnapAngleButtonWidth, SnapAngleButtonHeight));
		bool bSnapAngleClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapAngleHovered = ImGui::IsItemHovered();

		// 버튼 배경 그리기
		ImDrawList* SnapAngleDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapAngleBgColor = bSnapAngleHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			SnapAngleBgColor = IM_COL32(38, 38, 38, 255);
		}
		SnapAngleDrawList->AddRectFilled(SnapAngleButtonPos, ImVec2(SnapAngleButtonPos.x + SnapAngleButtonWidth, SnapAngleButtonPos.y + SnapAngleButtonHeight), SnapAngleBgColor, 4.0f);
		SnapAngleDrawList->AddRect(SnapAngleButtonPos, ImVec2(SnapAngleButtonPos.x + SnapAngleButtonWidth, SnapAngleButtonPos.y + SnapAngleButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// 텍스트 그리기
		ImVec2 SnapAngleTextPos = ImVec2(
			SnapAngleButtonPos.x + SnapAnglePadding,
			SnapAngleButtonPos.y + (SnapAngleButtonHeight - ImGui::GetTextLineHeight()) * 0.5f
		);
		SnapAngleDrawList->AddText(SnapAngleTextPos, IM_COL32(220, 220, 220, 255), SnapAngleText);

		if (bSnapAngleClicked)
		{
			ImGui::OpenPopup("##RotationSnapAnglePopup");
		}

		// 각도 선택 팝업
		if (ImGui::BeginPopup("##RotationSnapAnglePopup"))
		{
			ImGui::Text("Rotation Snap Angle");
			ImGui::Separator();

			constexpr float SnapAngles[] = { 5.0f, 10.0f, 15.0f, 22.5f, 30.0f, 45.0f, 60.0f, 90.0f };
			constexpr const char* SnapAngleLabels[] = { "5°", "10°", "15°", "22.5°", "30°", "45°", "60°", "90°" };

			for (int i = 0; i < IM_ARRAYSIZE(SnapAngles); ++i)
			{
				const bool bIsSelected = (std::abs(SnapAngle - SnapAngles[i]) < 0.1f);
				if (ImGui::MenuItem(SnapAngleLabels[i], nullptr, bIsSelected))
				{
					SnapAngle = SnapAngles[i];
				}
			}

			ImGui::EndPopup();
		}

		if (bSnapAngleHovered)
		{
			ImGui::SetTooltip("Choose rotation snap angle");
		}
	}
}

void UViewportToolbarWidgetBase::RenderScaleSnapControls(bool& bSnapEnabled, float& SnapValue)
{
	ImGui::SameLine(0.0f, 8.0f);

	// 스케일 스냅 토글 버튼 (아이콘)
	if (IconSnapScale && IconSnapScale->GetTextureSRV())
	{
		constexpr float SnapToggleButtonSize = 24.0f;
		constexpr float SnapToggleIconSize = 16.0f;

		ImVec2 SnapToggleButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##ScaleSnapToggle", ImVec2(SnapToggleButtonSize, SnapToggleButtonSize));
		bool bSnapToggleClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapToggleHovered = ImGui::IsItemHovered();

		ImDrawList* SnapToggleDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapToggleBgColor;
		if (bSnapEnabled)
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(40, 40, 40, 255) : IM_COL32(20, 20, 20, 255);
		}
		else
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		}
		if (ImGui::IsItemActive())
		{
			SnapToggleBgColor = IM_COL32(50, 50, 50, 255);
		}
		SnapToggleDrawList->AddRectFilled(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBgColor, 4.0f);

		// 테두리
		ImU32 SnapToggleBorderColor = bSnapEnabled ? IM_COL32(150, 150, 150, 255) : IM_COL32(96, 96, 96, 255);
		SnapToggleDrawList->AddRect(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBorderColor, 4.0f);

		// 아이콘 렌더링 (중앙 정렬)
		ImVec2 IconMin = ImVec2(
			SnapToggleButtonPos.x + (SnapToggleButtonSize - SnapToggleIconSize) * 0.5f,
			SnapToggleButtonPos.y + (SnapToggleButtonSize - SnapToggleIconSize) * 0.5f
		);
		ImVec2 IconMax = ImVec2(IconMin.x + SnapToggleIconSize, IconMin.y + SnapToggleIconSize);
		ImU32 IconTintColor = bSnapEnabled ? IM_COL32(46, 163, 255, 255) : IM_COL32(220, 220, 220, 255);
		SnapToggleDrawList->AddImage((void*)IconSnapScale->GetTextureSRV(), IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), IconTintColor);

		if (bSnapToggleClicked)
		{
			bSnapEnabled = !bSnapEnabled;
		}

		if (bSnapToggleHovered)
		{
			ImGui::SetTooltip("Toggle scale snap");
		}
	}

	ImGui::SameLine(0.0f, 4.0f);

	// 스케일 스냅 값 선택 버튼
	{
		char SnapValueText[16];
		(void)snprintf(SnapValueText, sizeof(SnapValueText), "%.4g", SnapValue);

		constexpr float SnapValueButtonHeight = 24.0f;
		constexpr float SnapValuePadding = 8.0f;
		const ImVec2 SnapValueTextSize = ImGui::CalcTextSize(SnapValueText);
		const float SnapValueButtonWidth = SnapValueTextSize.x + SnapValuePadding * 2;

		ImVec2 SnapValueButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##ScaleSnapValue", ImVec2(SnapValueButtonWidth, SnapValueButtonHeight));
		bool bSnapValueClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapValueHovered = ImGui::IsItemHovered();

		// 버튼 배경 그리기
		ImDrawList* SnapValueDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapValueBgColor = bSnapValueHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			SnapValueBgColor = IM_COL32(38, 38, 38, 255);
		}
		SnapValueDrawList->AddRectFilled(SnapValueButtonPos, ImVec2(SnapValueButtonPos.x + SnapValueButtonWidth, SnapValueButtonPos.y + SnapValueButtonHeight), SnapValueBgColor, 4.0f);
		SnapValueDrawList->AddRect(SnapValueButtonPos, ImVec2(SnapValueButtonPos.x + SnapValueButtonWidth, SnapValueButtonPos.y + SnapValueButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// 텍스트 그리기
		ImVec2 SnapValueTextPos = ImVec2(
			SnapValueButtonPos.x + SnapValuePadding,
			SnapValueButtonPos.y + (SnapValueButtonHeight - ImGui::GetTextLineHeight()) * 0.5f
		);
		SnapValueDrawList->AddText(SnapValueTextPos, IM_COL32(220, 220, 220, 255), SnapValueText);

		if (bSnapValueClicked)
		{
			ImGui::OpenPopup("##ScaleSnapValuePopup");
		}

		// 값 선택 팝업
		if (ImGui::BeginPopup("##ScaleSnapValuePopup"))
		{
			ImGui::Text("Scale Snap Value");
			ImGui::Separator();

			constexpr float SnapValues[] = { 10.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.1f, 0.0625f, 0.03125f };
			constexpr const char* SnapValueLabels[] = { "10", "1", "0.5", "0.25", "0.125", "0.1", "0.0625", "0.03125" };

			for (int i = 0; i < IM_ARRAYSIZE(SnapValues); ++i)
			{
				const bool bIsSelected = (std::abs(SnapValue - SnapValues[i]) < 0.0001f);
				if (ImGui::MenuItem(SnapValueLabels[i], nullptr, bIsSelected))
				{
					SnapValue = SnapValues[i];
				}
			}

			ImGui::EndPopup();
		}

		if (bSnapValueHovered)
		{
			ImGui::SetTooltip("Choose scale snap value");
		}
	}
}

void UViewportToolbarWidgetBase::RenderCameraSpeedButton(UCamera* Camera, float ButtonWidth)
{
	if (!Camera || !IconCamera || !IconCamera->GetTextureSRV())
	{
		return;
	}

	float CameraMoveSpeed = Camera->GetMoveSpeed();
	char CameraSpeedText[16];
	(void)snprintf(CameraSpeedText, sizeof(CameraSpeedText), "%.1f", CameraMoveSpeed);

	constexpr float CameraSpeedButtonHeight = 24.0f;
	constexpr float CameraSpeedPadding = 8.0f;
	constexpr float CameraSpeedIconSize = 16.0f;

	ImVec2 CameraSpeedButtonPos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##CameraSpeedButton", ImVec2(ButtonWidth, CameraSpeedButtonHeight));
	bool bCameraSpeedClicked = ImGui::IsItemClicked();
	bool bCameraSpeedHovered = ImGui::IsItemHovered();

	ImDrawList* CameraSpeedDrawList = ImGui::GetWindowDrawList();
	ImU32 CameraSpeedBgColor = bCameraSpeedHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
	if (ImGui::IsItemActive())
	{
		CameraSpeedBgColor = IM_COL32(38, 38, 38, 255);
	}
	CameraSpeedDrawList->AddRectFilled(CameraSpeedButtonPos, ImVec2(CameraSpeedButtonPos.x + ButtonWidth, CameraSpeedButtonPos.y + CameraSpeedButtonHeight), CameraSpeedBgColor, 4.0f);
	CameraSpeedDrawList->AddRect(CameraSpeedButtonPos, ImVec2(CameraSpeedButtonPos.x + ButtonWidth, CameraSpeedButtonPos.y + CameraSpeedButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

	// 아이콘 (왼쪽)
	const ImVec2 CameraSpeedIconPos = ImVec2(
		CameraSpeedButtonPos.x + CameraSpeedPadding,
		CameraSpeedButtonPos.y + (CameraSpeedButtonHeight - CameraSpeedIconSize) * 0.5f
	);
	CameraSpeedDrawList->AddImage(
		IconCamera->GetTextureSRV(),
		CameraSpeedIconPos,
		ImVec2(CameraSpeedIconPos.x + CameraSpeedIconSize, CameraSpeedIconPos.y + CameraSpeedIconSize)
	);

	// 텍스트 (오른쪽 정렬)
	const ImVec2 CameraSpeedTextSize = ImGui::CalcTextSize(CameraSpeedText);
	const ImVec2 CameraSpeedTextPos = ImVec2(
		CameraSpeedButtonPos.x + ButtonWidth - CameraSpeedTextSize.x - CameraSpeedPadding,
		CameraSpeedButtonPos.y + (CameraSpeedButtonHeight - ImGui::GetTextLineHeight()) * 0.5f
	);
	CameraSpeedDrawList->AddText(CameraSpeedTextPos, IM_COL32(220, 220, 220, 255), CameraSpeedText);

	if (bCameraSpeedClicked)
	{
		ImGui::OpenPopup("##CameraSettingsPopup");
	}

	if (bCameraSpeedHovered)
	{
		ImGui::SetTooltip("Camera Settings");
	}
}

void UViewportToolbarWidgetBase::RenderViewModeButton(FViewportClient* Client, const char** ViewModeLabels)
{
	if (!Client || !IconLitCube || !IconLitCube->GetTextureSRV())
	{
		return;
	}

	EViewModeIndex CurrentMode = Client->GetViewMode();
	int32 CurrentModeIndex = static_cast<int32>(CurrentMode);
	constexpr float ViewModeButtonHeight = 24.0f;
	constexpr float ViewModeIconSize = 16.0f;
	constexpr float ViewModePadding = 4.0f;
	const ImVec2 ViewModeTextSize = ImGui::CalcTextSize(ViewModeLabels[CurrentModeIndex]);
	const float ViewModeButtonWidth = ViewModePadding + ViewModeIconSize + ViewModePadding + ViewModeTextSize.x + ViewModePadding;

	ImVec2 ViewModeButtonPos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##ViewModeButton", ImVec2(ViewModeButtonWidth, ViewModeButtonHeight));
	bool bViewModeClicked = ImGui::IsItemClicked();
	bool bViewModeHovered = ImGui::IsItemHovered();

	ImDrawList* ViewModeDrawList = ImGui::GetWindowDrawList();
	ImU32 ViewModeBgColor = bViewModeHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
	if (ImGui::IsItemActive())
	{
		ViewModeBgColor = IM_COL32(38, 38, 38, 255);
	}
	ViewModeDrawList->AddRectFilled(ViewModeButtonPos, ImVec2(ViewModeButtonPos.x + ViewModeButtonWidth, ViewModeButtonPos.y + ViewModeButtonHeight), ViewModeBgColor, 4.0f);
	ViewModeDrawList->AddRect(ViewModeButtonPos, ImVec2(ViewModeButtonPos.x + ViewModeButtonWidth, ViewModeButtonPos.y + ViewModeButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

	// LitCube 아이콘
	const ImVec2 ViewModeIconPos = ImVec2(ViewModeButtonPos.x + ViewModePadding, ViewModeButtonPos.y + (ViewModeButtonHeight - ViewModeIconSize) * 0.5f);
	ViewModeDrawList->AddImage(
		IconLitCube->GetTextureSRV(),
		ViewModeIconPos,
		ImVec2(ViewModeIconPos.x + ViewModeIconSize, ViewModeIconPos.y + ViewModeIconSize)
	);

	// 텍스트
	const ImVec2 ViewModeTextPos = ImVec2(ViewModeButtonPos.x + ViewModePadding + ViewModeIconSize + ViewModePadding, ViewModeButtonPos.y + (ViewModeButtonHeight - ImGui::GetTextLineHeight()) * 0.5f);
	ViewModeDrawList->AddText(ViewModeTextPos, IM_COL32(220, 220, 220, 255), ViewModeLabels[CurrentModeIndex]);

	if (bViewModeClicked)
	{
		ImGui::OpenPopup("##ViewModePopup");
	}

	// ViewMode 팝업
	if (ImGui::BeginPopup("##ViewModePopup"))
	{
		for (int i = 0; i < 7; ++i)
		{
			if (IconLitCube && IconLitCube->GetTextureSRV())
			{
				ImGui::Image(IconLitCube->GetTextureSRV(), ImVec2(16, 16));
				ImGui::SameLine();
			}

			if (ImGui::MenuItem(ViewModeLabels[i], nullptr, i == CurrentModeIndex))
			{
				Client->SetViewMode(static_cast<EViewModeIndex>(i));
			}
		}
		ImGui::EndPopup();
	}

	if (bViewModeHovered)
	{
		ImGui::SetTooltip("View Mode");
	}
}

void UViewportToolbarWidgetBase::RenderViewTypeButton(FViewportClient* Client, const char** ViewTypeLabels, float ButtonWidth, bool bIsPilotMode, const char* PilotedActorName)
{
	if (!Client)
	{
		return;
	}

	EViewType CurrentViewType = Client->GetViewType();
	int32 CurrentViewTypeIndex = static_cast<int32>(CurrentViewType);

	UTexture* ViewTypeIcons[7] = { IconPerspective, IconTop, IconBottom, IconLeft, IconRight, IconFront, IconBack };
	UTexture* CurrentViewTypeIcon = ViewTypeIcons[CurrentViewTypeIndex];

	constexpr float ButtonHeight = 24.0f;
	constexpr float IconSize = 16.0f;
	constexpr float Padding = 4.0f;

	// 파일럿 모드일 경우 액터 이름 표시, 아니면 ViewType 표시
	const char* DisplayText = ViewTypeLabels[CurrentViewTypeIndex];
	char TruncatedActorName[128] = "";

	if (bIsPilotMode && PilotedActorName && strlen(PilotedActorName) > 0)
	{
		// 최대 표시 가능한 텍스트 폭 계산
		const float MaxTextWidth = 250.0f - Padding - IconSize - Padding * 2;
		const ImVec2 ActorNameSize = ImGui::CalcTextSize(PilotedActorName);

		if (ActorNameSize.x > MaxTextWidth)
		{
			// 텍스트가 너무 길면 "..." 축약
			size_t Len = strlen(PilotedActorName);
			while (Len > 3)
			{
				snprintf(TruncatedActorName, sizeof(TruncatedActorName), "%.*s...", static_cast<int>(Len - 3), PilotedActorName);
				ImVec2 TruncatedSize = ImGui::CalcTextSize(TruncatedActorName);
				if (TruncatedSize.x <= MaxTextWidth)
				{
					break;
				}
				Len--;
			}
			DisplayText = TruncatedActorName;
		}
		else
		{
			DisplayText = PilotedActorName;
		}
	}

	ImVec2 ButtonPos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##ViewTypeButton", ImVec2(ButtonWidth, ButtonHeight));
	bool bClicked = ImGui::IsItemClicked();
	bool bHovered = ImGui::IsItemHovered();

	// 버튼 배경 (파일럿 모드면 강조 색상)
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImU32 BgColor;
	ImU32 BorderColor;
	ImU32 TextColor;

	if (bIsPilotMode)
	{
		BgColor = bHovered ? IM_COL32(40, 60, 80, 255) : IM_COL32(30, 50, 70, 255);
		if (ImGui::IsItemActive()) BgColor = IM_COL32(50, 70, 90, 255);
		BorderColor = IM_COL32(100, 150, 200, 255);
		TextColor = IM_COL32(180, 220, 255, 255);
	}
	else
	{
		BgColor = bHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive()) BgColor = IM_COL32(38, 38, 38, 255);
		BorderColor = IM_COL32(96, 96, 96, 255);
		TextColor = IM_COL32(220, 220, 220, 255);
	}

	DrawList->AddRectFilled(ButtonPos, ImVec2(ButtonPos.x + ButtonWidth, ButtonPos.y + ButtonHeight), BgColor, 4.0f);
	DrawList->AddRect(ButtonPos, ImVec2(ButtonPos.x + ButtonWidth, ButtonPos.y + ButtonHeight), BorderColor, 4.0f);

	// 아이콘
	if (CurrentViewTypeIcon && CurrentViewTypeIcon->GetTextureSRV())
	{
		const ImVec2 IconPos = ImVec2(ButtonPos.x + Padding, ButtonPos.y + (ButtonHeight - IconSize) * 0.5f);
		DrawList->AddImage(
			CurrentViewTypeIcon->GetTextureSRV(),
			IconPos,
			ImVec2(IconPos.x + IconSize, IconPos.y + IconSize)
		);
	}

	// 텍스트
	const ImVec2 TextPos = ImVec2(ButtonPos.x + Padding + IconSize + Padding, ButtonPos.y + (ButtonHeight - ImGui::GetTextLineHeight()) * 0.5f);
	DrawList->AddText(TextPos, TextColor, DisplayText);

	if (bClicked)
	{
		ImGui::OpenPopup("##ViewTypeDropdown");
	}

	// ViewType 드롭다운
	if (ImGui::BeginPopup("##ViewTypeDropdown"))
	{
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

		// 파일럿 모드 항목 (최상단, 선택된 상태로 표시)
		if (bIsPilotMode && PilotedActorName && strlen(PilotedActorName) > 0)
		{
			if (IconPerspective && IconPerspective->GetTextureSRV())
			{
				ImGui::Image((ImTextureID)IconPerspective->GetTextureSRV(), ImVec2(16, 16));
				ImGui::SameLine();
			}

			ImGui::MenuItem(PilotedActorName, nullptr, true, false); // 선택됨, 비활성화
			ImGui::Separator();
		}

		// 일반 ViewType 항목들
		for (int i = 0; i < 7; ++i)
		{
			if (ViewTypeIcons[i] && ViewTypeIcons[i]->GetTextureSRV())
			{
				ImGui::Image((ImTextureID)ViewTypeIcons[i]->GetTextureSRV(), ImVec2(16, 16));
				ImGui::SameLine();
			}

			bool bIsCurrentViewType = (i == CurrentViewTypeIndex && !bIsPilotMode);
			if (ImGui::MenuItem(ViewTypeLabels[i], nullptr, bIsCurrentViewType))
			{
				EViewType NewType = static_cast<EViewType>(i);
				Client->SetViewType(NewType);

				// Update camera type
				if (UCamera* Camera = Client->GetCamera())
				{
					if (NewType == EViewType::Perspective)
					{
						Camera->SetCameraType(ECameraType::ECT_Perspective);
					}
					else
					{
						Camera->SetCameraType(ECameraType::ECT_Orthographic);
					}
				}
			}
		}

		ImGui::PopStyleColor();
		ImGui::EndPopup();
	}
}
