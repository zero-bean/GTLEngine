#include "pch.h"
#include "Render/RenderPass/Public/CameraPostProcessPass.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Render/Renderer/Public/DeviceResources.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"

FCameraPostProcessPass::FCameraPostProcessPass(UPipeline* InPipeline, ID3D11DepthStencilState* InDS, ID3D11BlendState* InBS)
	: FRenderPass(InPipeline), DS(InDS), BS(InBS)
{
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/BlitVS.hlsl", {}, &VertexShader, nullptr);
	FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/FadePS.hlsl", &PixelShader);
	CBRenderTarget = FRenderResourceFactory::CreateConstantBuffer<FVector2>();
	CBFade = FRenderResourceFactory::CreateConstantBuffer<FVector4>();
}

void FCameraPostProcessPass::SetRenderTargets(UDeviceResources* DeviceResources)
{
	ID3D11RenderTargetView* RenderTargetViews[] = { DeviceResources->GetDestinationRTV() };
	Pipeline->SetRenderTargets(1, RenderTargetViews, DeviceResources->GetDepthBufferDSV());
}

void FCameraPostProcessPass::Execute(FRenderingContext& Context)
{
	AGameMode* GameMode = GWorld->GetGameMode();
	if (GameMode == nullptr) { return; }
	APlayerCameraManager* PCM = GameMode->GetPlayerCameraManager();
	if (PCM == nullptr) { return; }

	// -- Fade Section -- //
	auto RS = FRenderResourceFactory::GetRasterizerState({ ECullMode::None, EFillMode::Solid });
	FPipelineInfo PipelineInfo = { nullptr, VertexShader, RS, DS, PixelShader, BS };
	Pipeline->UpdatePipeline(PipelineInfo);
	FRenderResourceFactory::UpdateConstantBufferData(CBRenderTarget, Context.RenderTargetSize);
	Pipeline->SetConstantBuffer(0, EShaderType::PS, CBRenderTarget);

	if (PCM->GetCameraFadeAmount() > 0.001f)
	{
		const FVector FadeColor = PCM->GetCameraFadeColor();
		const FVector4 FadeData(FadeColor.X, FadeColor.Y, FadeColor.Z, PCM->GetCameraFadeAmount());
		FRenderResourceFactory::UpdateConstantBufferData(CBFade, FadeData);
		Pipeline->SetConstantBuffer(1, EShaderType::PS, CBFade);
		Pipeline->Draw(3, 0);
	}
	// -- Fade Section -- //
}

void FCameraPostProcessPass::Release()
{
	SafeRelease(VertexShader);
	SafeRelease(PixelShader);
	SafeRelease(CBRenderTarget);
	SafeRelease(CBFade);
}
