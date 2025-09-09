#include "pch.h"

#include "Editor/Public/Editor.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/Axis.h"
#include "Core/Public/Object.h"
#include "Editor/Public/ObjectPicker.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"


UEditor::UEditor() = default;
UEditor::~UEditor() = default;

void UEditor::Update(HWND WindowHandle)
{
	auto& Renderer = URenderer::GetInstance(); 
	Camera.Update();
	Camera.UpdateMatrix();
	ObjectPicker.PickActor(ULevelManager::GetInstance().GetCurrentLevel(), WindowHandle, Camera);
	
	Renderer.UpdateConstant(Camera.GetFViewProjConstants());
}
void UEditor::RenderEditor()
{
	Gizmo.RenderGizmo(ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor());
	Axis.Render();
	Grid.RenderGrid();
}

//void UEditor::RenderEditor()
//{
//
//	for (auto& PrimitiveComponent : ULevelManager::GetInstance().GetCurrentLevel()->GetEditorPrimitiveComponents())
//	{
//		if (!PrimitiveComponent) continue;
//
//		FPipelineInfo PipelineInfo = {
//			DefaultInputLayout,
//			DefaultVertexShader,
//			RasterizerState,
//			DepthStencilState,
//			DefaultPixelShader,
//			nullptr,
//			PrimitiveComponent->GetTopology()
//		};
//
//		Pipeline->UpdatePipeline(PipelineInfo);
//
//		Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
//		Renderer.UpdateConstant(
//			PrimitiveComponent);
//
//		Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
//		UpdateConstant(PrimitiveComponent->GetColor());
//
//		Pipeline->SetVertexBuffer(PrimitiveComponent->GetVertexBuffer(), Stride);
//		Pipeline->Draw(static_cast<UINT>(PrimitiveComponent->GetVerticesData()->size()), 0);
//	}
//}
