#include "pch.h"
#include "Render/UI/Widget/Public/CapsuleComponentWidget.h"
#include "Component/Public/CapsuleComponent.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Component/Public/ActorComponent.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(UCapsuleComponentWidget, UWidget)

void UCapsuleComponentWidget::Initialize()
{
}

void UCapsuleComponentWidget::Update()
{
	ULevel* CurrentLevel = GWorld->GetLevel();
	if (CurrentLevel)
	{
		UActorComponent* NewSelectedComponent = GEditor->GetEditorModule()->GetSelectedComponent();
		if (CapsuleComponent != NewSelectedComponent)
		{
			CapsuleComponent = Cast<UCapsuleComponent>(NewSelectedComponent);
		}
	}
}

void UCapsuleComponentWidget::RenderWidget()
{
	if (!CapsuleComponent)
	{
		return;
	}

	ImGui::Separator();
	ImGui::Text("Capsule Component");

	// Push black background style for input fields
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	// Capsule Radius
	float Radius = CapsuleComponent->GetCapsuleRadius();
	if (ImGui::DragFloat("Capsule Radius", &Radius, 0.01f, 0.0f, 1000.0f, "%.2f"))
	{
		CapsuleComponent->SetCapsuleRadius(Radius);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Radius of the capsule cylinder and hemispheres");
	}

	// Capsule Half Height
	float HalfHeight = CapsuleComponent->GetCapsuleHalfHeight();
	if (ImGui::DragFloat("Capsule Half Height", &HalfHeight, 0.01f, 0.0f, 1000.0f, "%.2f"))
	{
		CapsuleComponent->SetCapsuleHalfHeight(HalfHeight);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Distance from center to the top (excluding hemisphere)");
	}

	ImGui::PopStyleColor(3);

	// Collision Settings
	ImGui::Separator();
	ImGui::Text("Collision");

	bool bGenerateOverlap = CapsuleComponent->GetGenerateOverlapEvents();
	if (ImGui::Checkbox("Generate Overlap Events", &bGenerateOverlap))
	{
		CapsuleComponent->SetGenerateOverlapEvents(bGenerateOverlap);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("If enabled, this component will generate overlap events (BeginOverlap/EndOverlap)");
	}

	// Mobility
	const char* MobilityItems[] = { "Static", "Movable" };
	int CurrentMobility = (CapsuleComponent->GetMobility() == EComponentMobility::Static) ? 0 : 1;
	if (ImGui::Combo("Mobility", &CurrentMobility, MobilityItems, IM_ARRAYSIZE(MobilityItems)))
	{
		CapsuleComponent->SetMobility((CurrentMobility == 0) ? EComponentMobility::Static : EComponentMobility::Movable);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Static: Don't move during gameplay (overlap with Static skipped)\nMovable: Can move during gameplay (overlap checked every frame)");
	}
}
