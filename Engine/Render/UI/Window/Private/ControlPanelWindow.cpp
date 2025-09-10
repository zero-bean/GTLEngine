#include "pch.h"
#include "Render/UI/Window/Public/ControlPanelWindow.h"

#include "Editor/Public/Camera.h"
#include "Mesh/Public/Actor.h"
#include "Mesh/Public/CubeActor.h"
#include "Mesh/Public/SphereActor.h"
#include "Mesh/Public/TriangleActor.h"
#include "Mesh/Public/SquareActor.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Render/UI/Widget/Public/FPSWidget.h"

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

	AddWidget(new UFPSWidget());
}

/**
 * @brief 초기화 함수
 */
void UControlPanelWindow::Initialize()
{
	UE_LOG("ControlPanelWindow: Initialized");

	SelectedActor = nullptr;
	EditLocation = FVector(0, 0, 0);
	EditRotation = FVector(0, 0, 0);
	EditScale = FVector(1, 1, 1);
	bTransformChanged = false;

	LevelManager = &ULevelManager::GetInstance();
	StatusMessage = "";
	StatusMessageTimer = 0.0f;

	if (!CameraPtr) { return; }

	CameraModeIndex = (CameraPtr->GetCameraType() == ECameraType::ECT_Perspective) ? 0 : 1;
	UiFovY = CameraPtr->GetFovY();
	UiNearZ = CameraPtr->GetNearZ();
	UiFarZ = CameraPtr->GetFarZ();
}

// /**
//  * @brief 업데이트 함수
//  */
// void UControlPanelWindow::Update()
// {
//
//
// 	if (!CameraPtr) { return; }
//
// 	// Camera
// 	CameraPtr->SetCameraType(CameraModeIndex == 0
// 		                         ? ECameraType::ECT_Perspective
// 		                         : ECameraType::ECT_Orthographic);
//
// 	/*
// 	 * @brief 카메라 파라미터 설정
// 	 */
// 	UiNearZ = std::max(0.0001f, UiNearZ);
// 	UiFarZ = std::max(UiNearZ + 0.0001f, UiFarZ);
// 	UiFovY = std::min(170.0f, std::max(1.0f, UiFovY));
//
// 	CameraPtr->SetNearZ(UiNearZ);
// 	CameraPtr->SetFarZ(UiFarZ);
// 	CameraPtr->SetFovY(UiFovY);
//
// 	/*
// 	 * @brief 카메라 업데이트
// 	 */
// 	if (CameraModeIndex == 0)
// 	{
// 		CameraPtr->UpdateMatrixByPers();
// 	}
// 	else
// 	{
// 		CameraPtr->UpdateMatrixByOrth();
// 	}
// }

// /**
//  * @brief 렌더링
//  */
// void UControlPanelWindow::Render()
// {
// 	RenderPrimitiveSpawn();
// 	RenderLevelIO();
// 	RenderCamera();
//
// 	// 매 프레임 Level의 선택된 Actor를 확인
// 	ULevelManager& LevelManager = ULevelManager::GetInstance();
// 	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();
//
// 	if (CurrentLevel)
// 	{
// 		AActor* CurrentSelectedActor = CurrentLevel->GetSelectedActor();
// 		if (SelectedActor != CurrentSelectedActor)
// 		{
// 			SelectedActor = CurrentSelectedActor;
// 			if (SelectedActor)
// 			{
// 				UpdateTransformFromActor();
// 			}
// 		}
//
// 		LevelObjectCount = CurrentLevel->GetAllocatedCount();
// 		LevelObjectMemory = CurrentLevel->GetAllocatedBytes();
// 	}
//
//
// 	ImGui::Separator();
//
// 	//Memory Indicator
// 	RenderLevelMemorySection();
//
// 	// Spawn Indicator
// 	RenderActorSpawnSection();
// 	ImGui::Separator();
//
// 	// Actor Name & Pointer
// 	RenderActorInspection();
//
// 	// 액터가 선택된 경우에만 EditMode 및 관련 UI 표시
// 	if (HasValidActor())
// 	{
// 		ImGui::Separator();
//
// 		// Edit Mode
// 		RenderModeSelector();
// 		ImGui::Separator();
//
// 		switch (CurrentEditMode)
// 		{
// 		case EEditMode::Transform:
// 			RenderTransformEditor();
// 			break;
// 		case EEditMode::Components:
// 			RenderComponentEditor();
// 			break;
// 		case EEditMode::Properties:
// 			RenderPropertyEditor();
// 			break;
// 		}
//
// 		// Transform Reset
// 		if (ImGui::CollapsingHeader("Advanced Options"))
// 		{
// 			if (ImGui::Button("Reset Transform"))
// 			{
// 				SelectedActor->SetActorLocation(FVector(0, 0, 0));
// 				SelectedActor->SetActorRotation(FVector(0, 0, 0));
// 				SelectedActor->SetActorScale3D(FVector(1, 1, 1));
// 				UpdateTransformFromActor();
// 			}
// 		}
// 	}
//
// 	ImGui::Separator();
//
// 	RenderActorDeleteSection();
// }

void UControlPanelWindow::RenderPrimitiveSpawn()
{
	ImGui::Text("Primitive Actor 생성");

	// Primitive 타입 선택 드롭다운
	const char* PrimitiveTypes[] = {"Cube", "Sphere", "Triangle", "Square"}; // Triangle 제외
	ImGui::Text("Primitive Type:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	ImGui::Combo("##PrimitiveType", &SelectedPrimitiveType, PrimitiveTypes, 4);

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

	ImGui::Separator();
}

void UControlPanelWindow::RenderLevelIO()
{
	// New Level Section
	ImGui::Text("새로운 레벨 생성");
	ImGui::InputText("Level Name", NewLevelNameBuffer, sizeof(NewLevelNameBuffer));
	if (ImGui::Button("Create New Level", ImVec2(150, 30)))
	{
		CreateNewLevel();
	}

	// Status Message
	if (StatusMessageTimer > 0.0f)
	{
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), StatusMessage.c_str());
		StatusMessageTimer -= UTimeManager::GetInstance().GetDeltaTime();
	}
	// Reserve Space
	else
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
	}

	ImGui::Separator();

	// Save Section
	ImGui::Text("레벨 Save & Load");
	if (ImGui::Button("Save Level", ImVec2(120, 30)))
	{
		path FilePath = OpenSaveFileDialog();
		if (!FilePath.empty())
		{
			SaveLevel(FilePath.string());
		}
	}

	// ImGui::SameLine();
	// if (ImGui::Button("Quick Save", ImVec2(120, 30)))
	// {
	// 	// 빠른 저장 - 기본 경로에 저장
	// 	SaveLevel("");
	// }

	// Load Section
	ImGui::SameLine();
	if (ImGui::Button("Load Level", ImVec2(120, 30)))
	{
		path FilePath = OpenLoadFileDialog();
		if (!FilePath.empty())
		{
			LoadLevel(FilePath.string());
		}
	}
}

void UControlPanelWindow::RenderCamera()
{
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
	if (bSyncedOnce == false)
	{
		SyncFromCamera();
		bSyncedOnce = true;
	}

	// === Mouse (read-only)
	// {
        	//	// 	ImGui::TextUnformatted("Mouse");
        	//	// 	ImGui::Separator();
        	//	//
        	//	// 	const UInputManager& Input = UInputManager::GetInstance();
        	//	// 	const FVector& Pos = Input.GetMousePosition();
        	//	// 	const FVector& Delta = Input.GetMouseDelta();
        	//	//
        	//	// 	ImGui::Text("Position:  x=%.1f  y=%.1f", Pos.X, Pos.Y);
        	//	// 	ImGui::Text("Delta:     dx=%.2f dy=%.2f", Delta.X, Delta.Y);
        	//	// 	ImGui::Spacing();
        	//	// }

	// === Camera Mode & Optics
	{
		ImGui::TextUnformatted("Camera Optics");
		ImGui::Separator();

		// 모드 전환
		const char* Models[] = {"Perspective", "Orthographic"};
		if (ImGui::Combo("Mode", &CameraModeIndex, Models, IM_ARRAYSIZE(Models)))
		{
			PushToCamera();
		}

		// FOV: 원근에서 주로 사용, 직교에서는 화면 폭에만 영향(너는 Ortho에서도 FovY로 OrthoWidth 계산함)
		// 카메라 코드에 맞춰 그대로 노출
		bool bChanged = false;
		bChanged |= ImGui::SliderFloat("FOV Y (deg)", &UiFovY, 1.0f, 170.0f, "%.1f");

		// Near/Far
		bChanged |= ImGui::DragFloat("Z Near", &UiNearZ, 0.01f, 0.0001f, 1e6f, "%.4f");
		bChanged |= ImGui::DragFloat("Z Far", &UiFarZ, 0.1f, 0.001f, 1e7f, "%.3f");

		if (bChanged)
		{
			CameraPtr->SetFovY(UiFovY);
			CameraPtr->SetNearZ(UiNearZ);
			CameraPtr->SetFarZ(UiFarZ);
		}

		// 퀵 버튼
		if (ImGui::Button("Reset Optics"))
		{
			UiFovY = 80.0f;
			UiNearZ = 0.1f;
			UiFarZ = 1000.0f;
			PushToCamera();
		}
	}

	// === Camera Transform
	{
		ImGui::TextUnformatted("Camera Transform");
		ImGui::Separator();

		// 위치/회전은 참조로 직접 수정 (Camera가 참조 반환 제공)
		auto& Location = CameraPtr->GetLocation();
		auto& Rotation = CameraPtr->GetRotation();

		if (ImGui::DragFloat3("Position (X,Y,Z)", &Location.X, 0.05f))
		{
			// 위치 바뀌면 바로 뷰 갱신
			if (CameraModeIndex == 0) CameraPtr->UpdateMatrixByPers();
			else CameraPtr->UpdateMatrixByOrth();
		}

		// 회전(도 단위) 입력 및 보정
		bool RotChanged = false;
		RotChanged |= ImGui::DragFloat3("Rotation (Pitch,Yaw,Roll)", &Rotation.X, 0.1f);

		// 피치/야오 간단 보정
		if (Rotation.X > 89.0f) Rotation.X = 89.0f;
		if (Rotation.X < -89.0f) Rotation.X = -89.0f;
		if (Rotation.Y > 180.0f) Rotation.Y -= 360.0f;
		if (Rotation.Y < -180.0f) Rotation.Y += 360.0f;

		if (RotChanged)
		{
			if (CameraModeIndex == 0) CameraPtr->UpdateMatrixByPers();
			else CameraPtr->UpdateMatrixByOrth();
		}

		ImGui::Spacing();
	}
}

void UControlPanelWindow::RenderModeSelector()
{
	const char* ModeNames[] = {"Transform", "Components", "Properties"};
	int32 CurrentMode = static_cast<int32>(CurrentEditMode);

	if (ImGui::Combo("Edit Mode", &CurrentMode, ModeNames, 3))
	{
		CurrentEditMode = static_cast<EEditMode>(CurrentMode);
	}
}

void UControlPanelWindow::RenderLevelMemorySection()
{
	ImGui::Text("Level Object Memory");

	ImGui::LabelText("Count", "%d", LevelObjectCount);
	ImGui::LabelText("Byte", "%d", LevelObjectMemory);
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
void UControlPanelWindow::SpawnActors() const
{
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("ControlPanel: No Current Level To Spawn Actors");
		return;
	}

	UE_LOG("ControlPanel: Spawning %d Actors Of Type %s", NumberOfSpawn,
	       (SelectedPrimitiveType == 0 ? "Cube" : "Sphere"));

	// 지정된 개수만큼 액터 생성
	for (int32 i = 0; i < NumberOfSpawn; i++)
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
		else if (SelectedPrimitiveType == 2)
		{
			NewActor = CurrentLevel->SpawnActor<ATriangleActor>();
		}
		else if (SelectedPrimitiveType == 3)
		{
			NewActor = CurrentLevel->SpawnActor<ASquareActor>();
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

			UE_LOG("ControlPanel: Spawned Actor At (%.2f, %.2f, %.2f)", RandomX, RandomY, RandomZ);
		}
		else
		{
			UE_LOG("ControlPanel: Failed To Spawn Actor %d", i);
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
		UE_LOG("ControlPanel: No Actor Selected For Deletion");
		return;
	}

	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("ControlPanel: No Current Level To Delete Actor From");
		return;
	}

	UE_LOG("ControlPanel: Marking Selected Actor For Deletion: %p", SelectedActor);

	// 지연 삭제를 사용하여 안전하게 다음 틱에서 삭제
	CurrentLevel->MarkActorForDeletion(SelectedActor);

	// MarkActorForDeletion에서 선택 해제도 처리하므로 여기에서는 단순히 nullptr로 설정
	SelectedActor = nullptr;
	UE_LOG("ControlPanel: Actor Marked For Deletion In Next Tick");
}

/**
 * @brief Windows API를 활용한 파일 저장 Dialog Modal을 생성하는 UI Window 기능
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path UControlPanelWindow::OpenSaveFileDialog()
{
	OPENFILENAMEW ofn;
	wchar_t szFile[260] = {};

	// 기본 파일명 설정
	wcscpy_s(szFile, L"NewLevel.json");

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetActiveWindow(); // 현재 활성 윈도우를 부모로 설정
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
	ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.lpstrTitle = L"Save Level File";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"json";

	// Modal 다이얼로그 표시 - 이 함수가 리턴될 때까지 다른 입력 차단
	cout << "[LevelManagerWindow] Opening Save Dialog (Modal)..." << "\n";

	if (GetSaveFileNameW(&ofn) == TRUE)
	{
		cout << "[LevelManagerWindow] Save Dialog Closed - File Selected" << "\n";
		return path(szFile);
	}

	cout << "[LevelManagerWindow] Save Dialog Closed - Cancelled" << "\n";
	return L"";
}

/**
 * @brief Windows API를 사용하여 파일 열기 다이얼로그를 엽니다
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path UControlPanelWindow::OpenLoadFileDialog()
{
	OPENFILENAMEW ofn;
	wchar_t szFile[260] = {};

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetActiveWindow(); // 현재 활성 윈도우를 부모로 설정
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
	ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.lpstrTitle = L"Load Level File";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_HIDEREADONLY;

	// Modal 다이얼로그 표시
	cout << "[LevelManagerWindow] Opening Load Dialog (Modal)..." << "\n";

	if (GetOpenFileNameW(&ofn) == TRUE)
	{
		cout << "[LevelManagerWindow] Load Dialog Closed - File Selected" << "\n";
		return path(szFile);
	}

	cout << "[LevelManagerWindow] Load Dialog Closed - Cancelled" << "\n";
	return L"";
}

/**
 * @brief 지정된 경로에 Level을 Save하는 UI Window 기능
 * @param InFilePath 저장할 파일 경로
 */
void UControlPanelWindow::SaveLevel(const FString& InFilePath)
{
	try
	{
		bool bSuccess;

		if (InFilePath.empty())
		{
			// Quick Save인 경우 기본 경로 사용
			bSuccess = LevelManager->SaveCurrentLevel("");
		}
		else
		{
			bSuccess = LevelManager->SaveCurrentLevel(InFilePath);
		}

		if (bSuccess)
		{
			StatusMessage = "Level Saved Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Level Saved Successfully" << "\n";
		}
		else
		{
			StatusMessage = "Failed To Save Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Failed To Save Level" << "\n";
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = FString("Save Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		cout << "[LevelManagerWindow] Save Error: " << Exception.what() << "\n";
	}
}

/**
 * @brief 지정된 경로에서 Level File을 Load 하는 UI Window 기능
 * @param InFilePath 불러올 파일 경로
 */
void UControlPanelWindow::LoadLevel(const FString& InFilePath)
{
	try
	{
		// 파일명에서 확장자를 제외하고 레벨 이름 추출
		FString LevelName = InFilePath;

		size_t LastSlash = LevelName.find_last_of("\\/");
		if (LastSlash != std::wstring::npos)
		{
			LevelName = LevelName.substr(LastSlash + 1);
		}

		size_t LastDot = LevelName.find_last_of(".");
		if (LastDot != std::wstring::npos)
		{
			LevelName = LevelName.substr(0, LastDot);
		}

		bool bSuccess = LevelManager->LoadLevel(LevelName, InFilePath);

		if (bSuccess)
		{
			StatusMessage = "Level Loaded Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Level Loaded Successfully" << "\n";
		}
		else
		{
			StatusMessage = "Failed To Load Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Failed To Load Level" << "\n";
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = FString("Load Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		cout << "[LevelManagerWindow] Load Error: " << Exception.what() << "\n";
	}
}

/**
 * @brief New Blank Level을 생성하는 UI Window 기능
 */
void UControlPanelWindow::CreateNewLevel()
{
	try
	{
		FString LevelName = FString(NewLevelNameBuffer, NewLevelNameBuffer + strlen(NewLevelNameBuffer));

		if (LevelName.empty())
		{
			StatusMessage = "Please Enter A Level Name!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			return;
		}

		bool bSuccess = LevelManager->CreateNewLevel(LevelName);

		if (bSuccess)
		{
			StatusMessage = "New Level Created Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] New Level Created: " << FString(NewLevelNameBuffer) << "\n";
		}
		else
		{
			StatusMessage = "Failed To Create New Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Failed To Create New Level" << "\n";
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = std::string("Create Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		cout << "[LevelManagerWindow] Create Error: " << Exception.what() << "\n";
	}
}

void UControlPanelWindow::SyncFromCamera()
{
	if (!CameraPtr)
	{
		return;
	}

	CameraModeIndex = (CameraPtr->GetCameraType() == ECameraType::ECT_Perspective) ? 0 : 1;
	UiFovY = CameraPtr->GetFovY();
	UiNearZ = CameraPtr->GetNearZ();
	UiFarZ = CameraPtr->GetFarZ();
}

void UControlPanelWindow::PushToCamera()
{
	if (!CameraPtr)
	{
		return;
	}

	CameraPtr->SetCameraType(CameraModeIndex == 0
		                         ? ECameraType::ECT_Perspective
		                         : ECameraType::ECT_Orthographic);

	UiNearZ = max(0.0001f, UiNearZ);
	UiFarZ = max(UiNearZ + 0.0001f, UiFarZ);
	UiFovY = min(170.0f, max(1.0f, UiFovY));

	CameraPtr->SetNearZ(UiNearZ);
	CameraPtr->SetFarZ(UiFarZ);
	CameraPtr->SetFovY(UiFovY);

	if (CameraModeIndex == 0)
	{
		CameraPtr->UpdateMatrixByPers();
	}
	else
	{
		CameraPtr->UpdateMatrixByOrth();
	}
}
