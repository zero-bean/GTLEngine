#include "pch.h"
#include "Render/RenderPass/Public/ColorCopyPass.h"

#include "Render/Renderer/Public/RenderResourceFactory.h"

FColorCopyPass::FColorCopyPass(UPipeline* InPipeline, ID3D11DepthStencilState* InDS)
	: FRenderPass(InPipeline, nullptr, nullptr), DS(InDS)
{
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/BlitVS.hlsl", {}, &VertexShader, nullptr);
	FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/BlitPS.hlsl", &PixelShader);

	SamplerState = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
	CBRenderTarget = FRenderResourceFactory::CreateConstantBuffer<FVector2>();
}

void FColorCopyPass::SetRenderTargets(class UDeviceResources* DeviceResources)
{
	DeviceResources->SwapFrameBuffers();

	ID3D11RenderTargetView* RTVs[] = { DeviceResources->GetBackBufferRTV() };
	SceneSRV = DeviceResources->GetSourceSRV();
	Pipeline->SetRenderTargets(1, RTVs, nullptr);
}

void FColorCopyPass::Execute(FRenderingContext& Context)
{
	GPU_EVENT(URenderer::GetInstance().GetDeviceContext(), "ColorCopyPass");
	auto RS = FRenderResourceFactory::GetRasterizerState({ ECullMode::None, EFillMode::Solid });
	FPipelineInfo PipelineInfo = { nullptr, VertexShader, RS, DS, PixelShader, nullptr };
	Pipeline->UpdatePipeline(PipelineInfo);

	FRenderResourceFactory::UpdateConstantBufferData(CBRenderTarget, Context.RenderTargetSize);
	Pipeline->SetConstantBuffer(0, EShaderType::PS, CBRenderTarget);
	Pipeline->SetShaderResourceView(0, EShaderType::PS, SceneSRV);
	Pipeline->SetSamplerState(0, EShaderType::PS, SamplerState);

	// Fullscreen triangle
	Pipeline->Draw(3, 0);
	Pipeline->SetShaderResourceView(0, EShaderType::PS, nullptr);
}

void FColorCopyPass::Release()
{
	SafeRelease(VertexShader);
	SafeRelease(PixelShader);
	SafeRelease(SamplerState);
	SafeRelease(CBRenderTarget);
}
