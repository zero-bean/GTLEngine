#include "pch.h"
#include "Render/RenderPass/Public/CameraPrePass.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"

FCameraPrePass::FCameraPrePass(UPipeline* InPipeline)
	: FRenderPass(InPipeline, nullptr, nullptr)
{
}

void FCameraPrePass::SetRenderTargets(UDeviceResources* DeviceResources)
{
}

void FCameraPrePass::Execute(FRenderingContext& Context)
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	AGameMode* GameMode = GWorld->GetGameMode();
	if (!GameMode) { return; }

	APlayerCameraManager* PCM = GameMode->GetPlayerCameraManager();
	if (!PCM) { return; }
	const FPostProcessSettings& Settings = PCM->GetPostProcessSettings();

	D3D11_VIEWPORT DefaultViewport = Context.LocalViewport;
	D3D11_VIEWPORT FullLetterBoxViewport = DefaultViewport;
	D3D11_VIEWPORT FinalViewport = DefaultViewport; // 기본값은 원본

	float Alpha = Settings.LetterBoxAmount;
	if (Alpha > 0.001f)
	{
		const float ScreenWidth = DefaultViewport.Width;
		const float ScreenHeight = DefaultViewport.Height;
		const float CurrentAspect = ScreenWidth / ScreenHeight;
		const float TargetAspect = Settings.TargetAspectRatio;

		// (상하 레터박스)
		if (CurrentAspect < TargetAspect)
		{
			float NewHeight = ScreenWidth / TargetAspect;
			float BarHeight = (ScreenHeight - NewHeight) / 2.0f;

			// [수정] BarHeight를 더해줍니다.
			FullLetterBoxViewport.TopLeftY = DefaultViewport.TopLeftY + BarHeight;
			FullLetterBoxViewport.Height = NewHeight;
		}
		// (좌우 레터박스)
		else if (CurrentAspect > TargetAspect)
		{
			float NewWidth = ScreenHeight * TargetAspect;
			float BarWidth = (ScreenWidth - NewWidth) / 2.0f;

			// [수정] BarWidth를 더해줍니다.
			FullLetterBoxViewport.TopLeftX = DefaultViewport.TopLeftX + BarWidth;
			FullLetterBoxViewport.Width = NewWidth;
		}

		// Lerp는 이제 올바른 값을 기준으로 보간합니다.
		FinalViewport.TopLeftY = Lerp(DefaultViewport.TopLeftY, FullLetterBoxViewport.TopLeftY, Alpha);
		FinalViewport.Height   = Lerp(DefaultViewport.Height, FullLetterBoxViewport.Height, Alpha);
		FinalViewport.TopLeftX = Lerp(DefaultViewport.TopLeftX, FullLetterBoxViewport.TopLeftX, Alpha);
		FinalViewport.Width    = Lerp(DefaultViewport.Width, FullLetterBoxViewport.Width, Alpha);
	}

	Context.LocalViewport = FinalViewport;
}

void FCameraPrePass::Release()
{
}
