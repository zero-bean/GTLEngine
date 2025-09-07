#include "pch.h"
#include "Render/UI/Window/Public/ActorInspectorWindow.h"

#include "Mesh/Public/Actor.h"

/**
 * @brief 생성자
 */
UActorInspectorWindow::UActorInspectorWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Actor Inspector";
	Config.DefaultSize = ImVec2(320, 400);
	Config.DefaultPosition = ImVec2(10, 10);
	Config.MinSize = ImVec2(280, 250);
	Config.DockDirection = EUIDockDirection::Left;
	Config.Priority = 15;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);
}

/**
 * @brief 초기화
 */
void UActorInspectorWindow::Initialize()
{
	cout << "[ActorInspectorWindow] Initialized" << endl;

	SelectedActor = nullptr;
	EditLocation = FVector(0, 0, 0);
	EditRotation = FVector(0, 0, 0);
	EditScale = FVector(1, 1, 1);
	bTransformChanged = false;
}

/**
 * @brief 렌더링
 */
void UActorInspectorWindow::Render()
{
	if (!HasValidActor())
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Actor Selected");
		ImGui::Text("Select an actor to edit its properties.");
		return;
	}

	// Actor 정보 표시
	ImGui::Text("Selected Actor: %p", SelectedActor);
	ImGui::Separator();

	// 편집 모드 선택
	RenderModeSelector();

	ImGui::Separator();

	// 현재 모드에 따라 다른 에디터 표시
	switch (CurrentEditMode)
	{
	case EEditMode::Transform:
		RenderTransformEditor();
		break;
	case EEditMode::Components:
		RenderComponentEditor();
		break;
	case EEditMode::Properties:
		RenderPropertyEditor();
		break;
	}

	// 고급 옵션
	if (ImGui::CollapsingHeader("Advanced Options"))
	{
		if (ImGui::Button("Reset Transform"))
		{
			SelectedActor->SetActorLocation(FVector(0, 0, 0));
			SelectedActor->SetActorRotation(FVector(0, 0, 0));
			SelectedActor->SetActorScale3D(FVector(1, 1, 1));
			UpdateTransformFromActor();
		}

		ImGui::SameLine();
		if (ImGui::Button("Duplicate Actor"))
		{
			// TODO: Actor 복제 기능
			ImGui::OpenPopup("Not Implemented");
		}

		if (ImGui::BeginPopup("Not Implemented"))
		{
			ImGui::Text("This feature is not implemented yet.");
			ImGui::EndPopup();
		}
	}
}

void UActorInspectorWindow::RenderModeSelector()
{
	const char* ModeNames[] = {"Transform", "Components", "Properties"};
	int CurrentMode = static_cast<int>(CurrentEditMode);

	if (ImGui::Combo("Edit Mode", &CurrentMode, ModeNames, 3))
	{
		CurrentEditMode = static_cast<EEditMode>(CurrentMode);
	}
}

void UActorInspectorWindow::RenderTransformEditor()
{
	UpdateTransformFromActor();

	ImGui::Text("Transform:");

	// Position
	ImGui::Text("Position");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	bool LocChanged = ImGui::DragFloat("X##Pos", &EditLocation.X, 0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	LocChanged |= ImGui::DragFloat("Y##Pos", &EditLocation.Y, 0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	LocChanged |= ImGui::DragFloat("Z##Pos", &EditLocation.Z, 0.1f);

	// Rotation
	ImGui::Text("Rotation");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	bool RotChanged = ImGui::DragFloat("X##Rot", &EditRotation.X, 0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	RotChanged |= ImGui::DragFloat("Y##Rot", &EditRotation.Y, 0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	RotChanged |= ImGui::DragFloat("Z##Rot", &EditRotation.Z, 0.1f);

	// Scale
	ImGui::Text("Scale   ");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	bool ScaleChanged = ImGui::DragFloat("X##Scale", &EditScale.X, 0.01f, 0.01f, 10.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	ScaleChanged |= ImGui::DragFloat("Y##Scale", &EditScale.Y, 0.01f, 0.01f, 10.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	ScaleChanged |= ImGui::DragFloat("Z##Scale", &EditScale.Z, 0.01f, 0.01f, 10.0f);

	// 변경사항 적용
	if (LocChanged || RotChanged || ScaleChanged)
	{
		bTransformChanged = true;
		ApplyTransformToActor();
	}

	// Uniform Scale 옵션
	static bool bUniformScale = false;
	if (ImGui::Checkbox("Uniform Scale", &bUniformScale))
	{
		if (bUniformScale)
		{
			float AvgScale = (EditScale.X + EditScale.Y + EditScale.Z) / 3.0f;
			EditScale = FVector(AvgScale, AvgScale, AvgScale);
			ApplyTransformToActor();
		}
	}
}

void UActorInspectorWindow::RenderComponentEditor()
{
	ImGui::Text("Components:");
	ImGui::Text("(Component editing not implemented yet)");

	// TODO: 컴포넌트 시스템이 구현되면 여기에 컴포넌트 편집 UI 추가
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Mesh Component");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Collision Component");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Physics Component");
}

void UActorInspectorWindow::RenderPropertyEditor()
{
	ImGui::Text("Properties:");
	ImGui::Text("(Property editing not implemented yet)");

	// TODO: 리플렉션 시스템이 구현되면 여기에 속성 편집 UI 추가
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Material Properties");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Physics Properties");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Custom Properties");
}

void UActorInspectorWindow::UpdateTransformFromActor()
{
	if (HasValidActor())
	{
		EditLocation = SelectedActor->GetActorLocation();
		EditRotation = SelectedActor->GetActorRotation();
		EditScale = SelectedActor->GetActorScale3D();
	}
}

void UActorInspectorWindow::ApplyTransformToActor()
{
	if (HasValidActor())
	{
		SelectedActor->SetActorLocation(EditLocation);
		SelectedActor->SetActorRotation(EditRotation);
		SelectedActor->SetActorScale3D(EditScale);
	}
}
