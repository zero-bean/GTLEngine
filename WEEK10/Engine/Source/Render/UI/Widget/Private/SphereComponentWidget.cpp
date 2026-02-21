#include "pch.h"
#include "Render/UI/Widget/Public/SphereComponentWidget.h"
#include "Component/Public/SphereComponent.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Component/Public/ActorComponent.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(USphereComponentWidget, UWidget)

void USphereComponentWidget::Initialize()
{
}

void USphereComponentWidget::Update()
{
	ULevel* CurrentLevel = GWorld->GetLevel();
	if (CurrentLevel)
	{
		UActorComponent* NewSelectedComponent = GEditor->GetEditorModule()->GetSelectedComponent();
		if (SphereComponent != NewSelectedComponent)
		{
			SphereComponent = Cast<USphereComponent>(NewSelectedComponent);
		}
	}
}

void USphereComponentWidget::RenderWidget()
{
	if (!SphereComponent)
	{
		return;
	}

	ImGui::Separator();
	ImGui::Text("Sphere Component");

	// Push black background style for input fields
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	// Sphere Radius
	float Radius = SphereComponent->GetSphereRadius();
	if (ImGui::DragFloat("Sphere Radius", &Radius, 0.01f, 0.0f, 1000.0f, "%.2f"))
	{
		SphereComponent->SetSphereRadius(Radius);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Radius of the sphere");
	}

	ImGui::PopStyleColor(3);

	// Collision Settings
	ImGui::Separator();
	ImGui::Text("Collision");

	bool bGenerateOverlap = SphereComponent->GetGenerateOverlapEvents();
	if (ImGui::Checkbox("Generate Overlap Events", &bGenerateOverlap))
	{
		SphereComponent->SetGenerateOverlapEvents(bGenerateOverlap);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("If enabled, this component will generate overlap events (BeginOverlap/EndOverlap)");
	}

	// Mobility
	const char* MobilityItems[] = { "Static", "Movable" };
	int CurrentMobility = (SphereComponent->GetMobility() == EComponentMobility::Static) ? 0 : 1;
	if (ImGui::Combo("Mobility", &CurrentMobility, MobilityItems, IM_ARRAYSIZE(MobilityItems)))
	{
		SphereComponent->SetMobility((CurrentMobility == 0) ? EComponentMobility::Static : EComponentMobility::Movable);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Static: Don't move during gameplay (overlap with Static skipped)\nMovable: Can move during gameplay (overlap checked every frame)");
	}
}
