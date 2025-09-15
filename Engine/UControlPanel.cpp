#include "stdafx.h"
#include "UControlPanel.h"
#include "USceneComponent.h"
#include "UCamera.h"
#include "USceneManager.h"
#include "UScene.h"
#include "UDefaultScene.h"
#include "UNamePool.h"
#include "UQuadComponent.h"
#include "URenderer.h"
#include "ShowFlagManager.h"

// 활성화(선택) 상태면 버튼색을 Active 계열로 바꿔서 '눌린 버튼'처럼 보이게 하는 헬퍼
static bool ModeButton(const char* Label, bool active, const ImVec2& size = ImVec2(0, 0))
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 colBtn = active ? style.Colors[ImGuiCol_ButtonActive] : style.Colors[ImGuiCol_Button];
	ImVec4 colHover = active ? style.Colors[ImGuiCol_ButtonActive] : style.Colors[ImGuiCol_ButtonHovered];
	ImVec4 colActive = style.Colors[ImGuiCol_ButtonActive];

	ImGui::PushStyleColor(ImGuiCol_Button, colBtn);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colHover);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, colActive);
	bool pressed = ImGui::Button(Label, size);
	ImGui::PopStyleColor(3);
	return pressed;
}

UControlPanel::UControlPanel(USceneManager* sceneManager, UGizmoManager* gizmoManager, UShowFlagManager* InShowFlagManager)
	: ImGuiWindowWrapper("Control Panel", ImVec2(0, 0), ImVec2(290, 500)), SceneManager(sceneManager), GizmoManager(gizmoManager), ShowFlagManager(InShowFlagManager)
{
	for (const auto& registeredType : UClass::GetClassList())
	{
		if (!registeredType->IsChildOrSelfOf(USceneComponent::StaticClass()))
			continue;

		FString displayName = registeredType->GetMeta("DisplayName");
		if (displayName.empty())
			continue;

		registeredTypes.push_back(registeredType.get());
		choiceStrList.push_back(registeredType->GetMeta("DisplayName"));
	}

	for (const FString& str : choiceStrList)
	{
		choices.push_back(str.c_str());
	}
}

void UControlPanel::RenderContent()
{
	PrimaryInformationSection();
	ImGui::Separator();
	SpawnPrimitiveSection();
	ImGui::Separator();
	SceneManagementSection();
	ImGui::Separator();
	CameraManagementSection();
	ImGui::Separator();
	ViewModeSection();
	ImGui::Separator();
	GridAndAxisSection(); // ⬅️ 추가
	ImGui::Separator();
	ShowFlagSection();
}

void UControlPanel::PrimaryInformationSection()
{
	float frameRate = ImGui::GetIO().Framerate;
	ImGui::Text("FPS %.0f (%.0f ms)", frameRate, 1000.0f / frameRate);
}

USceneComponent* UControlPanel::CreateSceneComponentFromChoice(int index)
{
	auto obj = registeredTypes[primitiveChoiceIndex]->CreateDefaultObject();
	if (!obj) return nullptr;
	return obj->Cast<USceneComponent>();
}

void UControlPanel::SpawnPrimitiveSection()
{
	ImGui::SetNextItemWidth(150);
	ImGui::Combo("Type", &primitiveChoiceIndex, choices.data(), static_cast<int32>(choices.size()));

	int32 objectCount = SceneManager->GetScene()->GetObjectCount();
	if (ImGui::Button("Spawn"))
	{
		USceneComponent* sceneComponent = CreateSceneComponentFromChoice(primitiveChoiceIndex);
		if (sceneComponent != nullptr)
		{
			sceneComponent->SetPosition(FVector(
				-5.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f,
				-5.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f,
				-5.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f
			));
			sceneComponent->SetScale(FVector(
				0.1f + static_cast<float>(rand()) / RAND_MAX * 0.7f,
				0.1f + static_cast<float>(rand()) / RAND_MAX * 0.7f,
				0.1f + static_cast<float>(rand()) / RAND_MAX * 0.7f
			));
			sceneComponent->SetRotation(FVector(
				-90.0f + static_cast<float>(rand()) / RAND_MAX * 180.0f,
				-90.0f + static_cast<float>(rand()) / RAND_MAX * 180.0f,
				-90.0f + static_cast<float>(rand()) / RAND_MAX * 180.0f
			));
			SceneManager->GetScene()->AddObject(sceneComponent);
		}
		UE_LOG("Spawned new object: %s", sceneComponent->Name.ToString().c_str());
		// 쿼드 추가
		USceneComponent* quad = NewObject<UQuadComponent>();
		SceneManager->GetScene()->AddObject(quad);
	}
	ImGui::SameLine();
	ImGui::BeginDisabled();
	ImGui::SetNextItemWidth(60);
	ImGui::SameLine();
	ImGui::InputInt("Spawned", &objectCount, 0);
	ImGui::EndDisabled();
}

void UControlPanel::SceneManagementSection()
{
	ImGui::Text("Scene Name");                // Label on top
	ImGui::SetNextItemWidth(200);             // Optional: set input width
	ImGui::InputText("##SceneNameInput", sceneName, sizeof(sceneName)); // invisible label

	if (ImGui::Button("New scene"))
	{
		// TODO : Make New Scene
		SceneManager->SetScene(NewObject<UDefaultScene>());

	}
	ImGui::SameLine();
	if (ImGui::Button("Save scene") && strcmp(sceneName, "") != 0)
	{
		std::filesystem::path _path("./data/");
		std::filesystem::create_directory(_path);
		SceneManager->SaveScene(_path.string() + FString(sceneName) + ".Scene");
	}
	ImGui::SameLine();
	if (ImGui::Button("Load scene") && strcmp(sceneName, "") != 0)
	{
		SceneManager->LoadScene("./data/" + FString(sceneName) + ".Scene");
	}
}

void UControlPanel::CameraManagementSection()
{
	UCamera* camera = SceneManager->GetScene()->GetCamera();
	// 카메라 정보
	FVector pos = camera->GetLocation();
	float cameraLocation[3] = { pos.X, pos.Y, pos.Z };
	float pitchYawRoll[3] = { 0 };
	camera->GetPitchYawDegrees(pitchYawRoll[0], pitchYawRoll[1]);
	FVector right, forward, up;
	camera->GetBasis(right, forward, up);

	// 보기용 버퍼 (Drag로 편집 못 하게 Read-only/Disabled 처리)
	float vRight[3] = { right.X,   right.Y,   right.Z };
	float vForward[3] = { forward.X, forward.Y, forward.Z };
	float vUp[3] = { up.X,      up.Y,      up.Z };
	// --- 테이블 UI ---
	bool locCommitted = false;
	bool rotCommitted = false;

	bool isOrthogonal = camera->IsOrtho();
	ImGui::Checkbox("Orthogonal", &isOrthogonal);
	if (isOrthogonal)
	{
		// 원하는 직교 크기로 (예시: 월드 단위 10x10)
		camera->SetOrtho(camera->GetOrthoW(), camera->GetOrthoH(), camera->GetNearZ(), camera->GetFarZ());
	}
	else
	{
		camera->SetPerspectiveDegrees(camera->GetFOV(),
			camera->GetAspect(), camera->GetNearZ(), camera->GetFarZ());
	}

	// === FOV (perspective일 때만 활성화) ===
	ImGui::TextUnformatted("FOV");

	float fovDeg = camera->GetFOV();

	const float minFov = 10.0f;
	const float maxFov = 120.0f;
	const float dragSpeed = 0.2f; // 픽셀당 증가량(°)

	float avail = ImGui::GetContentRegionAvail().x;

	ImGui::BeginDisabled(isOrthogonal);
	ImGui::PushID("FOV");

	// 드래그 박스 (좌우 드래그로 값 변경)
	ImGui::SetNextItemWidth(avail * 0.55f);
	bool fovChanged = ImGui::DragFloat("##fov_value", &fovDeg, dragSpeed, minFov, maxFov, "%.1f",
		ImGuiSliderFlags_AlwaysClamp);

	// 리셋 버튼(옵션)
	ImGui::SameLine();
	if (ImGui::Button("Reset")) { fovDeg = 60.0f; fovChanged = true; }

	if (fovChanged) {
		fovDeg = std::clamp(fovDeg, minFov, maxFov);
		camera->SetFOV(fovDeg); // 내부에서 proj 재빌드
		// (2) 즉시 저장
		CEditorIni::Get().SetFloat("Camera", "FOV", fovDeg);
		CEditorIni::Get().Save();
	}

	ImGui::PopID();
	ImGui::EndDisabled();



	// --- Euler(XYZ) 편집 ---
	// Location
	ImGui::Text("Camera Location");
	if (ImGui::BeginTable("EditableCameraTable", 3, ImGuiTableFlags_None))
	{
		ImGui::TableNextRow();
		for (int32 i = 0; i < 3; i++)
		{
			ImGui::TableSetColumnIndex(i);
			ImGui::SetNextItemWidth(-1);

			// DragFloat로 교체
			if (ImGui::DragFloat(("##loc" + std::to_string(i)).c_str(),
				&cameraLocation[i], 0.1f, -FLT_MAX, FLT_MAX, "%.3f"))
			{
				locCommitted = true; // 값이 바뀐 순간 바로 commit
			}
			// 만약 "편집 종료 후만" commit 원하면 IsItemDeactivatedAfterEdit() 체크로 바꾸기
		}
		ImGui::EndTable();
	}
	ImGui::Text("Camera Rotation");
	if (ImGui::BeginTable("EditableCameraTable", 3, ImGuiTableFlags_None))
	{
		ImGui::TableNextRow();
		for (int32 i = 0; i < 3; i++)
		{
			ImGui::TableSetColumnIndex(i);
			ImGui::SetNextItemWidth(-1);

			// DragFloat로 교체
			if (ImGui::DragFloat(("##loc" + std::to_string(i)).c_str(),
				&pitchYawRoll[i], 0.1f, -FLT_MAX, FLT_MAX, "%.3f"))
			{
				rotCommitted = true; // 값이 바뀐 순간 바로 commit
			}
			// 만약 "편집 종료 후만" commit 원하면 IsItemDeactivatedAfterEdit() 체크로 바꾸기
		}
		ImGui::EndTable();
	}


	float cameraSpeed = camera->GetMoveSpeed();
	const float minSpeed = 0.1f;
	const float maxSpeed = 100.0f;

	ImGui::Text("Camera Speed");
	// 드래그 박스 (좌우 드래그로 값 변경)
	ImGui::SetNextItemWidth(avail * 0.55f);
	bool speedChanged = ImGui::DragFloat("##cam_speed", &cameraSpeed, dragSpeed, minSpeed, maxSpeed, "%.001f",
		ImGuiSliderFlags_AlwaysClamp);

	if (speedChanged) {
		cameraSpeed = std::clamp(cameraSpeed, minSpeed, maxSpeed);
		camera->SetMoveSpeed(cameraSpeed); // 내부에서 proj 재빌드
		// (4) 즉시 저장
		CEditorIni::Get().SetFloat("Camera", "MoveSpeed", cameraSpeed);
		CEditorIni::Get().Save();
	}
	// --- Camera Basis 출력 ---
	ImGui::SeparatorText("Camera Basis");

	ImGui::Text("Right:   %.3f, %.3f, %.3f", right.X, right.Y, right.Z);
	ImGui::Text("Forward: %.3f, %.3f, %.3f", forward.X, forward.Y, forward.Z);
	ImGui::Text("Up:      %.3f, %.3f, %.3f", up.X, up.Y, up.Z);
	// World / Local 선택 (체크박스 대신, 버튼처럼 보이는 상호배타 토글)

	if (ModeButton("World", GizmoManager->GetIsWorldSpace()))
	{
		GizmoManager->SetGizmoSpace(true);
	}
	ImGui::SameLine();
	if (ModeButton("Local", !GizmoManager->GetIsWorldSpace()))
	{
		GizmoManager->SetGizmoSpace(false);
	}
	/* 모드/좌표계 토글 렌더링 */
	if (ModeButton("Translation", GizmoManager->GetTranslationType() == ETranslationType::Location))
	{
		GizmoManager->SetTranslationType(ETranslationType::Location);
	}
	ImGui::SameLine();
	if (ModeButton("Rotation", GizmoManager->GetTranslationType() == ETranslationType::Rotation))
	{
		GizmoManager->SetTranslationType(ETranslationType::Rotation);
	}
	ImGui::SameLine();
	if (ModeButton("Scale", GizmoManager->GetTranslationType() == ETranslationType::Scale))
	{
		GizmoManager->SetTranslationType(ETranslationType::Scale);
	}

	// === 변경사항을 카메라에 반영 ===
	// 위치
	// === 변경사항을 카메라에 '커밋 시'만 반영 ===
	if (locCommitted)
	{
		camera->SetLocation(FVector(cameraLocation[0], cameraLocation[1], cameraLocation[2]));
	}

	if (rotCommitted)
	{
		camera->SetPitchYawDegrees(pitchYawRoll[0], pitchYawRoll[1]);
	}
}

void UControlPanel::ViewModeSection()
{
	// 뷰모드 섹션
	ImGui::Text("View Mode");

	// 라디오 버튼 리스트로 현재 씬의 뷰모드 선택
	EViewModeIndex Current = SceneManager->GetScene()->GetViewMode();
	int32 Mode = static_cast<int32>(Current);
	bool bChanged = false;

	bChanged |= ImGui::RadioButton("Lit", &Mode, static_cast<int32>(EViewModeIndex::VMI_Lit));
	bChanged |= ImGui::RadioButton("Unlit", &Mode, static_cast<int32>(EViewModeIndex::VMI_Unlit));
	bChanged |= ImGui::RadioButton("Wireframe", &Mode, static_cast<int32>(EViewModeIndex::VMI_Wireframe));

	if (bChanged && Mode != static_cast<int32>(Current))
	{
		SceneManager->GetScene()->SetViewMode(static_cast<EViewModeIndex>(Mode));
	}
}

void UControlPanel::ShowFlagSection()
{
	ImGui::SeparatorText("Show Flags");

	if (ShowFlagManager == nullptr)
	{
		ImGui::TextUnformatted("ShowFlagManager not available");
		return;
	}

	auto FlagCheckbox = [&](const char* Label, EEngineShowFlags Flag)
		{
			bool enabled = ShowFlagManager->IsEnabled(Flag);
			// 체크 상태 표시 + 클릭 시 토글
			if (ImGui::Checkbox(Label, &enabled))
			{
				ShowFlagManager->Toggle(Flag);
			}
		};

	FlagCheckbox("Primitives", EEngineShowFlags::SF_Primitives);
	FlagCheckbox("BillboardText", EEngineShowFlags::SF_BillboardText);
}
void UControlPanel::GridAndAxisSection()
{
	if (!GizmoManager) return;

	ImGui::SeparatorText("Grid / Axis");

	bool showGrid = GizmoManager->GetShowGrid();
	bool showAxis = GizmoManager->GetShowAxis();
	if (ImGui::Checkbox("Show Grid", &showGrid)) GizmoManager->SetShowGrid(showGrid);
	ImGui::SameLine();
	if (ImGui::Checkbox("Show Axis", &showAxis)) GizmoManager->SetShowAxis(showAxis);

	float spacing = GizmoManager->GetGridSpacing();
	ImGui::SetNextItemWidth(80);
	if (ImGui::DragFloat("Grid Spacing", &spacing, 0.01f, 0.01f, 10000.0f, "%.3f",
		ImGuiSliderFlags_AlwaysClamp))
	{
		GizmoManager->SetGridSpacing(spacing);
		// editor.ini에 저장
		CEditorIni::Get().SetFloat("Grid", "Spacing", spacing);
		CEditorIni::Get().Save();
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset##gridspacing")) GizmoManager->SetGridSpacing(1.0f);

	int halfLines = GizmoManager->GetGridHalfLines();
	ImGui::SetNextItemWidth(80);
	if (ImGui::InputInt("Half Lines", &halfLines))
	{
		GizmoManager->SetGridHalfLines(halfLines); // 그 다음 프레임부터 새 개수로 생성/제출
	}

	//int boldEvery = GizmoManager->GetGridBoldEvery();
	//ImGui::SetNextItemWidth(80);
	//if (ImGui::DragInt("Bold Every N", &boldEvery, 1, 1, 1000))
	//{
	//	GizmoManager->SetGridBoldEvery(boldEvery);
	//}

	//// (선택) 라이브 확인용
	//ImGui::TextDisabled("Live: spacing=%.3f half=%d bold=%d",
	//	GizmoManager->GetGridSpacing(), GizmoManager->GetGridHalfLines(), GizmoManager->GetGridBoldEvery());
}