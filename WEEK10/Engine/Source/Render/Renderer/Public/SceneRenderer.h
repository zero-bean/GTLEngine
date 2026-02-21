#pragma once

class FSceneView;
class FSceneViewFamily;
class FRHICommandList;
class IRenderPass;
class UWorld;

/**
 * @brief Scene 기반 렌더링을 제공하는 클래스
 * SceneView와 World 정보를 바탕으로 여러 렌더 패스를 조율하여 최종 이미지 생성
 * @note 내부적으로 현재 구색만 맞춰놓고 Renderer의 RenderPass를 활용
 */
class FSceneRenderer
{
public:
	// 정적 팩토리 메소드로 생성
	static FSceneRenderer* CreateSceneRenderer(const FSceneViewFamily& InViewFamily);
	~FSceneRenderer();

	// 렌더링 실행
	void Render();
	void Cleanup();

	// CommandList 접근자
	FRHICommandList* GetCommandList() const { return CommandList; }

private:
	FSceneRenderer(const FSceneViewFamily& InViewFamily);

	void CreateDefaultRenderPasses();
	void RenderView(const FSceneView* InSceneView);

	const FSceneViewFamily* ViewFamily = nullptr;
	TArray<IRenderPass*> RenderPasses;
	FRHICommandList* CommandList = nullptr;
};
