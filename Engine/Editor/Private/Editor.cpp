#include "pch.h"

#include "Editor/Public/Editor.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/Axis.h"
#include "Core/Public/Object.h"
#include "Editor/Public/ObjectPicker.h"
#include "Core/Public/AppWindow.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Window/Public/CameraPanelWindow.h"


UEditor::UEditor()
	:Camera(),
	ObjectPicker(Camera)
{
	if (UCameraPanelWindow* Window =
		dynamic_cast<UCameraPanelWindow*>(UUIManager::GetInstance().FindUIWindow("Camera Control")))
	{
		Window->SetCamera(&Camera);
	}
	ObjectPicker.SetCamera(Camera);
};

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

