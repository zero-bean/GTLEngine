#include "pch.h"
#include "Render/RenderPass/Public/FogPass.h"

#include "Component/Public/HeightFogComponent.h"
#include "Editor/Public/Camera.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"

FFogPass::FFogPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferViewProj,
                   ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11DepthStencilState* InDS_Read,
                   ID3D11BlendState* InBlendState)
        : FRenderPass(InPipeline, InConstantBufferViewProj, nullptr),
            VS(InVS), PS(InPS), InputLayout(InLayout), DS_Read(InDS_Read), BlendState(InBlendState)
{
    ConstantBufferFog = FRenderResourceFactory::CreateConstantBuffer<FFogConstants>();
    ConstantBufferCameraInverse = FRenderResourceFactory::CreateConstantBuffer<FCameraInverseConstants>();
    ConstantBufferViewportInfo = FRenderResourceFactory::CreateConstantBuffer<FViewportConstants>();
}

void FFogPass::SetRenderTargets(class UDeviceResources* DeviceResources)
{
	// Fog는 원래 그려진 것에 Alpha Blend를 하는 방식이라 Frame Swap 필요 X
	ID3D11RenderTargetView* RTVs[] = { DeviceResources->GetDestinationRTV() };
	DepthSRV = DeviceResources->GetDepthBufferSRV();
	Pipeline->SetRenderTargets(1, RTVs, nullptr);
}

void FFogPass::Execute(FRenderingContext& Context)
{
	GPU_EVENT(URenderer::GetInstance().GetDeviceContext(), "FogPass");
    TIME_PROFILE(FogPass)

    if (!(Context.ShowFlags & EEngineShowFlags::SF_Fog)) return;

    //--- Get Renderer Singleton ---//
    URenderer& Renderer = URenderer::GetInstance();

    // --- Set Pipeline State --- //
    FPipelineInfo PipelineInfo = { InputLayout, VS, FRenderResourceFactory::GetRasterizerState({ ECullMode::Back, EFillMode::Solid }),
        DS_Read, PS, BlendState };
    Pipeline->UpdatePipeline(PipelineInfo);

	Pipeline->SetShaderResourceView(0, EShaderType::PS, Renderer.GetDepthBufferSRV());
	Pipeline->SetSamplerState(0, EShaderType::PS, Renderer.GetDefaultSampler());

    // --- Draw Fog --- //
    for (UHeightFogComponent* Fog : Context.Fogs)
    {
        // Fog가 보이지 않는 경우 건너뛴
        if (!Fog->GetVisible())
        {
            continue;
        }

        // Update Fog Constant Buffer (Slot 0)
        FFogConstants FogConstant;
        FVector color3 = Fog->GetFogInscatteringColor();
        FogConstant.FogColor = FVector4(color3.X, color3.Y, color3.Z, 1.0f);
        FogConstant.FogDensity = Fog->GetFogDensity();
        FogConstant.FogHeightFalloff = Fog->GetFogHeightFalloff();
        FogConstant.StartDistance = Fog->GetStartDistance();
        FogConstant.FogCutoffDistance = Fog->GetFogCutoffDistance();
        FogConstant.FogMaxOpacity = Fog->GetFogMaxOpacity();
        FogConstant.FogZ = Fog->GetWorldLocation().Z;
        FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferFog, FogConstant);
        Pipeline->SetConstantBuffer(0, EShaderType::PS, ConstantBufferFog);

        // Update CameraInverse Constant Buffer (Slot 1)
        FCameraInverseConstants CameraInverseConstants;
        CameraInverseConstants.ProjectionInverse = Context.ViewInfo.CameraConstants.Projection.Inverse();
        CameraInverseConstants.ViewInverse = Context.ViewInfo.CameraConstants.View.Inverse();
        FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferCameraInverse, CameraInverseConstants);
        Pipeline->SetConstantBuffer(1, EShaderType::PS, ConstantBufferCameraInverse);

        // Update ViewportInfo Constant Buffer (Slot 2)
        FViewportConstants ViewportConstants;
        ViewportConstants.RenderTargetSize = { Context.RenderTargetSize.X, Context.RenderTargetSize.Y };
        FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewportInfo, ViewportConstants);
        Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferViewportInfo);

        Pipeline->Draw(3,0);
    }
    Pipeline->SetShaderResourceView(0, EShaderType::PS, nullptr);
}

void FFogPass::Release()
{
    SafeRelease(ConstantBufferFog);
    SafeRelease(ConstantBufferCameraInverse);
    SafeRelease(ConstantBufferViewportInfo);
}
