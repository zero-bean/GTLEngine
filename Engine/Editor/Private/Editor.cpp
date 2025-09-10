#include "pch.h"
#include "Editor/Public/Editor.h"

#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/Axis.h"
#include "Editor/Public/ObjectPicker.h"
#include "Level/Public/Level.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/UI/Widget/Public/Widget.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Widget/Public/CameraControlWidget.h"

UEditor::UEditor()
	: ObjectPicker(Camera)
{
	ObjectPicker.SetCamera(Camera);

	// Set Camera to Control Panel
	auto& UIManager =UUIManager::GetInstance();
	auto* CameraControlWidget =
		reinterpret_cast<UCameraControlWidget*>(UIManager.FindWidget("Camera Control Widget"));
	CameraControlWidget->SetCamera(&Camera);
}

UEditor::~UEditor() = default;

void UEditor::Update()
{
	auto& Renderer = URenderer::GetInstance();
	Camera.Update();


	ObjectPicker.RayCast(ULevelManager::GetInstance().GetCurrentLevel(), Gizmo);
	Renderer.UpdateConstant(Camera.GetFViewProjConstants());
}
void UEditor::RenderEditor()
{
	Gizmo.RenderGizmo(ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor(), ObjectPicker);
	Grid.RenderGrid();
	Axis.Render();
}

