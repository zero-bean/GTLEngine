#include "pch.h"
#include "Render/Renderer/Public/SceneRenderer.h"
#include "Render/Renderer/Public/SceneView.h"
#include "Render/Renderer/Public/SceneViewFamily.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Level/Public/GameInstance.h"
#include "Render/UI/Viewport/Public/Viewport.h"

#if WITH_EDITOR
#include "Editor/Public/EditorPrimitive.h"
#include "Editor/Public/Gizmo.h"
#endif

/**
 * @brief SceneRenderer 정적 팩토리 메소드
 * ViewFamily마다 새로운 SceneRenderer 생성
 */
FSceneRenderer* FSceneRenderer::CreateSceneRenderer(const FSceneViewFamily& InViewFamily)
{
	return new FSceneRenderer(InViewFamily);
}

/**
 * @brief SceneRenderer 생성자
 * ViewFamily를 받아 렌더 패스들을 준비
 */
FSceneRenderer::FSceneRenderer(const FSceneViewFamily& InViewFamily)
	: ViewFamily(&InViewFamily)
{
	// TODO: RHICommandList 생성
	// CommandList = new FRHICommandList(GDynamicRHI);

	CreateDefaultRenderPasses();
}

/**
 * @brief SceneRenderer 소멸자
 */
FSceneRenderer::~FSceneRenderer()
{
	Cleanup();
}

/**
 * @brief 메인 렌더링 함수
 * ViewFamily의 모든 View를 순회하며 렌더링
 */
void FSceneRenderer::Render()
{
	if (!ViewFamily || !ViewFamily->IsValid())
	{
		UE_LOG_WARNING("SceneRenderer::Render - Invalid ViewFamily");
		return;
	}

	// ViewFamily에서 Views 추출
	const TArray<FSceneView*>& Views = ViewFamily->GetViews();

	for (FSceneView* SceneView : Views)
	{
		if (SceneView)
		{
			RenderView(SceneView);
		}
	}
}

/**
 * @brief 개별 View 렌더링
 * @param InSceneView 렌더링할 SceneView
 */
void FSceneRenderer::RenderView(const FSceneView* InSceneView)
{
	if (!InSceneView)
	{
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();

	// World 가져오기
	TObjectPtr<UWorld> World = InSceneView->GetWorld();

	if (!World)
	{
		UE_LOG_WARNING("SceneRenderer::RenderView - Invalid World");
		return;
	}

	// ViewRect 설정
	// const FRect& ViewRect = InSceneView->GetViewRect();

	// TODO: Viewport 설정
	// D3D11_VIEWPORT D3DViewport;
	// D3DViewport.TopLeftX = static_cast<float>(ViewRect.X);
	// D3DViewport.TopLeftY = static_cast<float>(ViewRect.Y);
	// D3DViewport.Width = static_cast<float>(ViewRect.W);
	// D3DViewport.Height = static_cast<float>(ViewRect.H);
	// D3DViewport.MinDepth = 0.0f;
	// D3DViewport.MaxDepth = 1.0f;
	// Renderer->GetDeviceContext()->RSSetViewports(1, &D3DViewport);

	// TODO: Depth/Stencil 클리어
	// Renderer->GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// TODO: 각 렌더 패스 실행
	// for (IRenderPass* Pass : RenderPasses)
	// {
	//     if (Pass && Pass->IsEnabled())
	//     {
	//         Pass->Execute(InSceneView, this);
	//     }
	// }

	// StandAlone Mode: RenderLevelForGameInstance 사용 (GEditor 의존성 없음)
	Renderer.RenderLevelForGameInstance(World, InSceneView, GameInstance);
}

/**
 * @brief 리소스 정리
 */
void FSceneRenderer::Cleanup()
{
	// TODO: RenderPass 정리
	// for (IRenderPass* Pass : RenderPasses)
	// {
	//     if (Pass)
	//     {
	//         Pass->Cleanup();
	//         delete Pass;
	//     }
	// }
	RenderPasses.Empty();

	// TODO: CommandList 정리
	// if (CommandList)
	// {
	//     delete CommandList;
	//     CommandList = nullptr;
	// }
}

/**
 * @brief 기본 렌더 패스 생성
 * TODO: FutureEngine의 렌더 패스 구조에 맞게 구현
 */
void FSceneRenderer::CreateDefaultRenderPasses()
{
	// TODO: FutureEngine의 RenderPass 구조에 맞게 구현
	//
	// 예시:
	// FDepthPrePass* DepthPass = new FDepthPrePass();
	// DepthPass->Initialize();
	// RenderPasses.Add(DepthPass);
	//
	// FBasePass* BasePass = new FBasePass();
	// BasePass->Initialize();
	// RenderPasses.Add(BasePass);
}
