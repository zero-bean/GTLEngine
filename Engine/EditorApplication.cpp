#include "stdafx.h"
#include "UApplication.h"
#include "EditorApplication.h"
#include "UMeshManager.h"
#include "ImguiConsole.h"
#include "UScene.h"
#include "UDefaultScene.h"
#include "URaycastManager.h"
#include "UGizmoArrowComp.h"
#include "UGizmoRotationHandleComp.h"
#include "UGizmoScaleHandleComp.h"

void EditorApplication::Update(float deltaTime)
{
	// Basic update logic
	UApplication::Update(deltaTime);
	gizmoManager.Update(deltaTime);
	// ★ AABB 표시 토글
	if (GetInputManager().IsKeyPressed('C')) {
		AABBManager.SetEnabled(!AABBManager.IsEnabled());
	}
	if (GetInputManager().IsKeyDown(VK_ESCAPE))
	{
		RequestExit();
	}

	if (GetInputManager().IsKeyPressed(VK_SPACE))
	{
		gizmoManager.NextTranslation();
	}

	if (GetInputManager().IsKeyPressed('X'))
	{
		gizmoManager.ChangeGizmoSpace();
	}

	FVector outImpactPoint;
	UGizmoComponent* hitGizmo = nullptr;
	UPrimitiveComponent* hitPrimitive = nullptr;

	if (GetInputManager().IsMouseButtonReleased(0))
	{
		TArray<UGizmoComponent*> gizmos;

		TArray<UGizmoComponent*>& g = gizmoManager.GetRaycastableGizmos();
		if (g.size() > 0)
		{
			for (UGizmoComponent* gizmo : g)
			{
				gizmos.push_back(gizmo);
				gizmo->bIsSelected = false;
			}
		}
		gizmoManager.EndDrag();
		return;
	}

	// 드래그 하고 있을때
	if (gizmoManager.IsDragging())
	{
		FRay ray = GetRaycastManager().CreateRayFromScreenPosition(GetSceneManager().GetScene()->GetCamera());
		gizmoManager.UpdateDrag(ray);
		// ★ AABB도 업데이트
		AABBManager.Update(deltaTime);
		return;
	}

	if (ImGui::GetIO().WantCaptureMouse) 
	{
		AABBManager.Update(deltaTime);
		return;
	}

	if (GetInputManager().IsMouseButtonPressed(0))
	{
		TArray<UPrimitiveComponent*> primitives;
		TArray<UGizmoComponent*> gizmos;

		TArray<UGizmoComponent*>& g = gizmoManager.GetRaycastableGizmos();
		if (g.size() > 0)
		{
			for (UGizmoComponent* gizmo : g)
			{
				gizmos.push_back(gizmo);
				gizmo->bIsSelected = false;
			}
		}

		for (UObject* obj : GetSceneManager().GetScene()->GetObjects())
		{
			if (UPrimitiveComponent* primitive = obj->Cast<UPrimitiveComponent>())
			{
				if (primitive->GetMesh()) primitives.push_back(primitive);
				primitive->bIsSelected = false;
			}
		}

		// std::cout << "gizmos.size() : " << gizmos.size();
		// std::cout << " primitives.size() : " << primitives.size() << std::endl;
		if (GetRaycastManager().RayIntersectsMeshes(GetSceneManager().GetScene()->GetCamera(), gizmos, hitGizmo, outImpactPoint))
		{
			// std::cout << "hitGizmo : " << hitGizmo << std::endl;
			if (auto target = gizmoManager.GetTarget())
			{
				target->bIsSelected = true;
				hitGizmo->bIsSelected = true;
				UGizmoArrowComp* arrow = hitGizmo->Cast<UGizmoArrowComp>();

				FRay ray = GetRaycastManager().CreateRayFromScreenPosition(GetSceneManager().GetScene()->GetCamera());

				// UGizmoArrowComp로 캐스팅 시도
				if (UGizmoArrowComp* arrow = hitGizmo->Cast<UGizmoArrowComp>())
				{
					gizmoManager.BeginDrag(ray, arrow->Axis, outImpactPoint, GetSceneManager().GetScene());
				}
				// UGizmoRotationHandleComp로 캐스팅 시도
				else if (UGizmoRotationHandleComp* rotationHandle = hitGizmo->Cast<UGizmoRotationHandleComp>())
				{
					gizmoManager.BeginDrag(ray, rotationHandle->Axis, outImpactPoint, GetSceneManager().GetScene()); // 스케일 드래그 시작 로직 추가
				}
				// UGizmoScaleHandleComp로 캐스팅 시도
				else if (UGizmoScaleHandleComp* scaleHandle = hitGizmo->Cast<UGizmoScaleHandleComp>())
				{
					gizmoManager.BeginDrag(ray, scaleHandle->Axis, outImpactPoint, GetSceneManager().GetScene()); // 스케일 드래그 시작 로직 추가
				}

				if (target->IsManageable())
					propertyWindow->SetTarget(target);
				// ★ 기즈모만 클릭한 경우에도 AABB는 타겟 기준 유지
				if (auto prim = target->Cast<UPrimitiveComponent>()) {
					AABBManager.SetTarget(prim);
					if (prim->GetMesh()) AABBManager.UseMeshLocalAABB(prim->GetMesh());
				}
			}
		}
		else if (GetRaycastManager().RayIntersectsMeshes(GetSceneManager().GetScene()->GetCamera(), primitives, hitPrimitive, outImpactPoint))
		{
			OnPrimitiveSelected(hitPrimitive);

			/*gizmoManager.SetTarget(hitPrimitive);
			hitPrimitive->bIsSelected = true;
			if (hitPrimitive->IsManageable())
				propertyWindow->SetTarget(hitPrimitive);
			AABBManager.SetTarget(hitPrimitive);
			if (hitPrimitive->GetMesh())
				AABBManager.UseMeshLocalAABB(hitPrimitive->GetMesh());
			else
				AABBManager.UseExplicitLocalAABB(FBoundingBox{ FVector(-0.5f,-0.5f,-0.5f), FVector(+0.5f,+0.5f,+0.5f) });*/
		}
		else
		{
			gizmoManager.SetTarget(nullptr);
			propertyWindow->SetTarget(nullptr);
			AABBManager.SetTarget(nullptr);
		}
	}

	// ★ 매 프레임 AABB 업데이트
	AABBManager.Update(deltaTime);
}

void EditorApplication::Render()
{
	UApplication::Render();
	gizmoManager.Draw(GetRenderer());
	// AABB 그리기
	AABBManager.Draw(GetRenderer());
}

void EditorApplication::RenderGUI()
{
	controlPanel->Render();
	SceneManagerPanel->Render();
	propertyWindow->Render();

	ImGui::SetNextWindowPos(ImVec2(0, 610));         // Fixed position (x=20, y=20)
	ImGui::SetNextWindowSize(ImVec2(290, 75));      // Fixed size (width=300, height=100)
	ImGui::Begin("Memory Stats", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse);                // Prevent resizing, moving, collapsing
	 
	ImGui::Text("Allocated Object Count : %d", UEngineStatics::GetTotalAllocationCount());
	ImGui::Text("Allocated Object Bytes : %d", UEngineStatics::GetTotalAllocationBytes());

	ImGui::End();

	bool isConsoleOpen = false;
	// static ImguiConsole imguiConsole;
	GConsole.Draw("Console", &isConsoleOpen);
}

bool EditorApplication::OnInitialize()
{
	UApplication::OnInitialize();
	// 리사이즈/초기화

	controlPanel = NewObject<UControlPanel>(&GetSceneManager(), &gizmoManager, &GetShowFlagManager());
	SceneManagerPanel = NewObject<USceneManagerPanel>(&GetSceneManager(), [this](UPrimitiveComponent* Prim) { OnPrimitiveSelected(Prim); });
	propertyWindow = NewObject<USceneComponentPropertyWindow>();

	if (!gizmoManager.Initialize(&GetMeshManager()))
	{
		MessageBox(GetWindowHandle(), L"Failed to initialize gizmo manager", L"Engine Error", MB_OK | MB_ICONERROR);
		return false;
	}
	gizmoManager.SetCamera(GetSceneManager().GetScene()->GetCamera());
	// ★ AABB 매니저 초기화
	if (!AABBManager.Initialize(&GetMeshManager()))
	{
		MessageBox(GetWindowHandle(), L"Failed to initialize AABB manager", L"Engine Error", MB_OK | MB_ICONERROR);
		return false;
	}
	AABBManager.SetEnabled(true);
	return true;
}


void EditorApplication::OnResize(int32 width, int32 height)
{
	UScene* scene = GetSceneManager().GetScene();
	if (scene == nullptr) return;

	UCamera* camera = scene->GetCamera();
	if (camera == nullptr) return;

	if (camera->IsOrtho())
	{
		camera->SetOrtho(camera->GetOrthoW(), camera->GetOrthoH(), camera->GetNearZ(), camera->GetFarZ());
	}
	else
	{
		camera->SetPerspectiveDegrees(
			camera->GetFOV(),
			(height > 0) ? (float)width / (float)height : 1.0f,
			camera->GetNearZ(),
			camera->GetFarZ());
	}
}

void EditorApplication::OnPrimitiveSelected(UPrimitiveComponent* Primitive)
{
	gizmoManager.SetTarget(Primitive);
	Primitive->bIsSelected = true;
	if (Primitive->IsManageable())
	{
		propertyWindow->SetTarget(Primitive);
	}

	AABBManager.SetTarget(Primitive);
	if (Primitive->GetMesh())
	{
		AABBManager.UseMeshLocalAABB(Primitive->GetMesh());
	}
	else
	{
		// 기본값의 큐브 넣어주기
		AABBManager.UseExplicitLocalAABB(FBoundingBox{ FVector(-0.5f,-0.5f,-0.5f), FVector(+0.5f,+0.5f,+0.5f) });
	}
}

UScene* EditorApplication::CreateDefaultScene()
{
	return NewObject<UDefaultScene>();
}

void EditorApplication::OnSceneChange()
{
	propertyWindow->SetTarget(nullptr);
	gizmoManager.SetCamera(GetSceneManager().GetScene()->GetCamera());
	gizmoManager.SetTarget(nullptr);

	// ★ 씬 변경 시 AABB 타겟 해제
	AABBManager.SetTarget(nullptr);
}
