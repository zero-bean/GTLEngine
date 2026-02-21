#include "pch.h"
#include "Render/UI/Widget/Public/CameraComponentWidget.h"
#include "Component/Public/CameraComponent.h"

IMPLEMENT_CLASS(UCameraComponentWidget, UWidget)

void UCameraComponentWidget::Update()
{
	ULevel* CurrentLevel = GWorld->GetLevel();
	if (CurrentLevel)
	{
		UActorComponent* NewSelectedComponent = GEditor->GetEditorModule()->GetSelectedComponent();
		if (CameraComponent != NewSelectedComponent)
		{
			CameraComponent = Cast<UCameraComponent>(NewSelectedComponent);
		}
	}
}

void UCameraComponentWidget::RenderWidget()
{
	if (!CameraComponent)
	{
		return;
	}

	// Black style for input fields
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

	// Camera Active checkbox
	bool bIsActive = CameraComponent->IsActive();
	if (ImGui::Checkbox("Is Active", &bIsActive))
	{
		CameraComponent->SetActive(bIsActive);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("카메라 활성화 여부\n활성화된 첫 번째 카메라가 렌더링에 사용됩니다.");
	}

	ImGui::PopStyleColor(4);

	// Camera Properties
	float FieldOfView = CameraComponent->GetFieldOfView();
	if (ImGui::DragFloat("Field of View", &FieldOfView, 1.0f, 1.0f, 179.0f, "%.1f"))
	{
		CameraComponent->SetFieldOfView(FieldOfView);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("시야각 (FOV)\n범위: 1.0 ~ 179.0");
	}

	float NearClip = CameraComponent->GetNearClipPlane();
	if (ImGui::DragFloat("Near Clip", &NearClip, 0.1f, 0.1f, FLT_MAX, "%.3f"))
	{
		CameraComponent->SetNearClipPlane(NearClip);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("근거리 클리핑 평면\n이 거리보다 가까운 물체는 렌더링되지 않습니다.");
	}

	float FarClip = CameraComponent->GetFarClipPlane();
	if (ImGui::DragFloat("Far Clip", &FarClip, 10.0f, NearClip + 1.0f, FLT_MAX, "%.1f"))
	{
		CameraComponent->SetFarClipPlane(FarClip);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("원거리 클리핑 평면\n이 거리보다 먼 물체는 렌더링되지 않습니다.");
	}

	// Projection Mode
	ECameraProjectionMode ProjectionMode = CameraComponent->GetProjectionMode();
	const char* ProjectionModes[] = { "Perspective", "Orthographic" };
	int CurrentMode = static_cast<int>(ProjectionMode);

	if (ImGui::Combo("Projection Mode", &CurrentMode, ProjectionModes, IM_ARRAYSIZE(ProjectionModes)))
	{
		CameraComponent->SetProjectionMode(static_cast<ECameraProjectionMode>(CurrentMode));
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("투영 모드\nPerspective: 원근감 있는 3D 뷰\nOrthographic: 2D 뷰 또는 등각 투영");
	}

	// Orthographic Width (only shown when Orthographic mode)
	if (ProjectionMode == ECameraProjectionMode::Orthographic)
	{
		float OrthoWidth = CameraComponent->GetOrthoWidth();
		if (ImGui::DragFloat("Ortho Width", &OrthoWidth, 1.0f, 1.0f, FLT_MAX, "%.1f"))
		{
			CameraComponent->SetOrthoWidth(OrthoWidth);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("직교 투영 너비\n높이는 종횡비에 따라 자동 계산됩니다.");
		}
	}

	ImGui::Separator();
}
