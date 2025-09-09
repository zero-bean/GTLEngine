#include "pch.h"
#include "Render/UI/Window/Public/ControlPanelWindow.h"

#include "Mesh/Public/Actor.h"
#include "Mesh/Public/CubeActor.h"
#include "Mesh/Public/SphereActor.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"

/**
 * @brief Control Panel 생성자
 */
UControlPanelWindow::UControlPanelWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Control Panel";
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
void UControlPanelWindow::Initialize()
{
	UE_LOG("[ControlPanelWindow] Initialized");

	SelectedActor = nullptr;
	EditLocation = FVector(0, 0, 0);
	EditRotation = FVector(0, 0, 0);
	EditScale = FVector(1, 1, 1);
	bTransformChanged = false;
}

/**
 * @brief 렌더링
 */
void UControlPanelWindow::Render()
{
	// 매 프레임 Level의 선택된 Actor를 확인
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (CurrentLevel)
	{
		AActor* CurrentSelectedActor = CurrentLevel->GetSelectedActor();
		if (SelectedActor != CurrentSelectedActor)
		{
			SelectedActor = CurrentSelectedActor;
			if (SelectedActor)
			{
				UpdateTransformFromActor();
			}
		}
	}

	// Spawn Indicator
	RenderActorSpawnSection();
	ImGui::Separator();

	// Actor Name & Pointer
	RenderActorInspection();

	// 액터가 선택된 경우에만 EditMode 및 관련 UI 표시
	if (HasValidActor())
	{
		ImGui::Separator();

		// Edit Mode
		RenderModeSelector();
		ImGui::Separator();

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

		// Transform Reset
		if (ImGui::CollapsingHeader("Advanced Options"))
		{
			if (ImGui::Button("Reset Transform"))
			{
				SelectedActor->SetActorLocation(FVector(0, 0, 0));
				SelectedActor->SetActorRotation(FVector(0, 0, 0));
				SelectedActor->SetActorScale3D(FVector(1, 1, 1));
				UpdateTransformFromActor();
			}
		}
	}

	ImGui::Separator();

	RenderActorDeleteSection();
}

void UControlPanelWindow::RenderModeSelector()
{
	const char* ModeNames[] = {"Transform", "Components", "Properties"};
	int CurrentMode = static_cast<int>(CurrentEditMode);

	if (ImGui::Combo("Edit Mode", &CurrentMode, ModeNames, 3))
	{
		CurrentEditMode = static_cast<EEditMode>(CurrentMode);
	}
}

void UControlPanelWindow::RenderActorInspection()
{
	// Actor Inspection
	if (!HasValidActor())
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Actor Selected");
		ImGui::Text("Select An Actor To Edit Its Properties.");
		return;
	}

	ImGui::Text("Selected Actor: %s (%p)",
	            !strcmp(SelectedActor->GetName().c_str(), "") ? "UnNamed" : SelectedActor->GetName().c_str(),
	            SelectedActor);
}

void UControlPanelWindow::RenderTransformEditor()
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

	// Uniform Scale 옵션
	static bool bUniformScale = false;
	if (ImGui::Checkbox("Uniform Scale", &bUniformScale))
	{
		if (bUniformScale)
		{
			// Uniform Scale을 체크할 때 기존 스케일의 평균값으로 설정
			float AvgScale = (EditScale.X + EditScale.Y + EditScale.Z) / 3.0f;
			EditScale = FVector(AvgScale, AvgScale, AvgScale);
			ApplyTransformToActor();
		}
	}

	// Scale
	ImGui::Text("Scale   ");
	ImGui::SameLine();

	bool ScaleChanged = false;
	if (bUniformScale)
	{
		// Uniform Scale 모드에서는 하나의 값으로 모든 축 제어
		ImGui::SetNextItemWidth(210);
		float UniformScale = EditScale.X;
		if (ImGui::DragFloat("Uniform##Scale", &UniformScale, 0.01f, 0.01f, 10.0f))
		{
			EditScale = FVector(UniformScale, UniformScale, UniformScale);
			ScaleChanged = true;
		}
	}
	else
	{
		// 개별 Scale 모드에서는 각 축을 개별 제어
		ImGui::SetNextItemWidth(70);
		ScaleChanged = ImGui::DragFloat("X##Scale", &EditScale.X, 0.01f, 0.01f, 10.0f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ScaleChanged |= ImGui::DragFloat("Y##Scale", &EditScale.Y, 0.01f, 0.01f, 10.0f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ScaleChanged |= ImGui::DragFloat("Z##Scale", &EditScale.Z, 0.01f, 0.01f, 10.0f);
	}

	// 변경사항 적용
	if (LocChanged || RotChanged || ScaleChanged)
	{
		bTransformChanged = true;
		ApplyTransformToActor();
	}
}

void UControlPanelWindow::RenderComponentEditor()
{
	ImGui::Text("Components:");
	ImGui::Text("(Component Editing Not Implemented Yet)");

	// TODO(KHJ): 컴포넌트 편집 UI 추가
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Mesh Component");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Collision Component");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Physics Component");
}

void UControlPanelWindow::RenderPropertyEditor()
{
	ImGui::Text("Properties:");
	ImGui::Text("(Property Editing Not Implemented Yet)");

	// TODO(KHJ): 속성 편집 UI 추가
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Material Properties");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Physics Properties");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Custom Properties");
}

void UControlPanelWindow::UpdateTransformFromActor()
{
	if (HasValidActor())
	{
		EditLocation = SelectedActor->GetActorLocation();
		EditRotation = SelectedActor->GetActorRotation();
		EditScale = SelectedActor->GetActorScale3D();
	}
}

void UControlPanelWindow::ApplyTransformToActor()
{
	if (HasValidActor())
	{
		SelectedActor->SetActorLocation(EditLocation);
		SelectedActor->SetActorRotation(EditRotation);
		SelectedActor->SetActorScale3D(EditScale);
	}
}

/**
 * @brief Actor Spawn Section Render
 */
void UControlPanelWindow::RenderActorSpawnSection()
{
	ImGui::Text("Actor Spawning");

	// Primitive 타입 선택 드롭다운
	const char* PrimitiveTypes[] = {"Cube", "Sphere"}; // Triangle 제외
	ImGui::Text("Primitive Type:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	ImGui::Combo("##PrimitiveType", &SelectedPrimitiveType, PrimitiveTypes, 2);

	// Spawn 버튼과 개수 입력
	ImGui::Text("Number of Spawn:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("##NumberOfSpawn", &NumberOfSpawn);
	NumberOfSpawn = max(1, NumberOfSpawn);
	NumberOfSpawn = min(100, NumberOfSpawn);

	ImGui::SameLine();
	if (ImGui::Button("Spawn Actors"))
	{
		SpawnActors();
	}

	// 스폰 범위 설정
	ImGui::Text("Spawn Range:");
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Min##SpawnRange", &SpawnRangeMin, 0.1f, -50.0f, SpawnRangeMax - 0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Max##SpawnRange", &SpawnRangeMax, 0.1f, SpawnRangeMin + 0.1f, 50.0f);
}

/**
 * @brief Actor Deletion Section Render
 */
void UControlPanelWindow::RenderActorDeleteSection()
{
	ImGui::Text("Actor Management");

	if (HasValidActor())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Selected: %p", SelectedActor);
		ImGui::SameLine();
		if (ImGui::Button("Delete Selected"))
		{
			DeleteSelectedActor();
		}
	}
	else
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Actor Selected For Deletion");
	}
}

/**
 * @brief Actor 생성 함수
 * 난수를 활용한 범위, 사이즈를 통한 생성 처리
 */
void UControlPanelWindow::SpawnActors()
{
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("[ControlPanel] No Current Level To Spawn Actors");
		return;
	}

	UE_LOG("[ControlPanel] Spawning %d Actors Of Type %s", NumberOfSpawn,
	       (SelectedPrimitiveType == 0 ? "Cube" : "Sphere"));

	// 지정된 개수만큼 액터 생성
	for (int i = 0; i < NumberOfSpawn; i++)
	{
		AActor* NewActor = nullptr;

		// 타입에 따라 액터 생성
		if (SelectedPrimitiveType == 0) // Cube
		{
			NewActor = CurrentLevel->SpawnActor<ACubeActor>();
		}
		else if (SelectedPrimitiveType == 1) // Sphere
		{
			NewActor = CurrentLevel->SpawnActor<ASphereActor>();
		}

		if (NewActor)
		{
			// 범위 내 랜덤 위치
			float RandomX = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomY = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomZ = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);

			NewActor->SetActorLocation(FVector(RandomX, RandomY, RandomZ));

			// 임의의 스케일 (0.5 ~ 2.0 범위)
			float RandomScale = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
			NewActor->SetActorScale3D(FVector(RandomScale, RandomScale, RandomScale));

			UE_LOG("[ControlPanel] Spawned Actor At (%.2f, %.2f, %.2f)", RandomX, RandomY, RandomZ);
		}
		else
		{
			UE_LOG("[ControlPanel] Failed To Spawn Actor %d", i);
		}
	}
}

/**
 * @brief Selected Actor 삭제 함수
 */
void UControlPanelWindow::DeleteSelectedActor()
{
	if (!HasValidActor())
	{
		UE_LOG("[ControlPanel] No Actor Selected For Deletion");
		return;
	}

	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("[ControlPanel] No Current Level To Delete Actor From");
		return;
	}

	UE_LOG("[ControlPanel] Marking Selected Actor For Deletion: %p", SelectedActor);

	// 지연 삭제를 사용하여 안전하게 다음 틱에서 삭제
	CurrentLevel->MarkActorForDeletion(SelectedActor);

	// MarkActorForDeletion에서 선택 해제도 처리하므로 여기에서는 단순히 nullptr로 설정
	SelectedActor = nullptr;
	UE_LOG("[ControlPanel] Actor Marked For Deletion In Next Tick");
}
