#include "pch.h"
#include "Render/UI/Widget/Public/LightSensorComponentWidget.h"
#include "Component/Public/LightSensorComponent.h"
#include "Component/Public/ActorComponent.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(ULightSensorComponentWidget, UWidget)

void ULightSensorComponentWidget::Initialize()
{
}

void ULightSensorComponentWidget::Update()
{
	ULevel* CurrentLevel = GWorld->GetLevel();
	if (CurrentLevel)
	{
		UActorComponent* NewSelectedComponent = GEditor->GetEditorModule()->GetSelectedComponent();
		if (SensorComponent != NewSelectedComponent)
		{
			SensorComponent = Cast<ULightSensorComponent>(NewSelectedComponent);
		}
	}
}

void ULightSensorComponentWidget::RenderWidget()
{
	if (!SensorComponent)
	{
		return;
	}

	ImGui::Separator();
	ImGui::PushItemWidth(150.0f);

	// 입력 필드 배경색을 검은색으로 설정
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	// === Sensor Status (Read-only) ===
	ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Sensor Status");
	ImGui::Separator();

	// Current Luminance (읽기 전용)
	float currentLuminance = SensorComponent->GetCurrentLuminance();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
	ImGui::Text("Current Luminance: %.4f", currentLuminance);
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("현재 측정된 조명 세기 (Luminance)\n0.0 = 완전한 어둠, 1.0 = 매우 밝음");
	}

	// Previous Luminance (읽기 전용)
	float previousLuminance = SensorComponent->GetPreviousLuminance();
	ImGui::Text("Previous Luminance: %.4f", previousLuminance);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("이전 프레임에서 측정된 Luminance 값");
	}

	// Luminance Change (계산)
	float luminanceChange = currentLuminance - previousLuminance;
	if (std::abs(luminanceChange) > 0.0001f)
	{
		ImVec4 changeColor = luminanceChange > 0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, changeColor);
		ImGui::Text("Change: %+.4f", luminanceChange);
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("이전 프레임 대비 Luminance 변화량\n초록색 = 밝아짐, 빨간색 = 어두워짐");
		}
	}
	else
	{
		ImGui::Text("Change: 0.0000");
	}

	ImGui::Spacing();
	ImGui::Separator();

	// === Sensor Settings ===
	ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Sensor Settings");
	ImGui::Separator();

	// Enabled Checkbox
	bool enabled = SensorComponent->GetEnabled();
	if (ImGui::Checkbox("Enabled", &enabled))
	{
		SensorComponent->SetEnabled(enabled);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("센서 활성화 여부\n비활성화 시 조명 측정을 중단합니다.");
	}

	// FramesPerRequest
	int32 framesPerRequest = SensorComponent->GetFramesPerRequest();
	if (ImGui::DragInt("Frames Per Request", &framesPerRequest, 1.0f, 2, 120))
	{
		SensorComponent->SetFramesPerRequest(framesPerRequest);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("몇 프레임마다 조명을 측정할지 설정\n최소: 2, 권장: 5~10\n값이 작을수록 정확하지만 성능 저하 가능");
	}

	ImGui::PopStyleColor(3);
	ImGui::PopItemWidth();
	ImGui::Separator();

	// === Performance Info ===
	ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Performance");
	ImGui::Text("Update Frequency: %.2f Hz", 60.0f / static_cast<float>(framesPerRequest));
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("초당 측정 횟수 (60 FPS 기준)");
	}
}
