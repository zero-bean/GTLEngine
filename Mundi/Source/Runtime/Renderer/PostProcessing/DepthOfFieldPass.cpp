#include "pch.h"
#include "DepthOfFieldPass.h"
#include "../SceneView.h"
#include "../../RHI/SwapGuard.h"

const char* FDepthOfFieldPass::DOF_CompositePSPath = "Shaders/PostProcess/DOF_Composite_PS.hlsl";
const char* FDepthOfFieldPass::DOF_GaussianBlurPSPath = "Shaders/PostProcess/DOF_GaussianBlur_PS.hlsl";
const char* FDepthOfFieldPass::DOF_CalcCOC_PSPath = "Shaders/PostProcess/DOF_CalcCOC_PS.hlsl";

void FDepthOfFieldPass::Execute(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice)
{
    //Common
    FSwapGuard Swap(RHIDevice, /*FirstSlot*/0, /*NumSlotsToUnbind*/2);
    RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
    RHIDevice->OMSetBlendState(false);
    ID3D11SamplerState* LinearClampSamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
    ID3D11SamplerState* PointClampSamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::PointClamp);
    ID3D11SamplerState* Smps[2] = { LinearClampSamplerState, PointClampSamplerState };
    RHIDevice->GetDeviceContext()->PSSetSamplers(0, 2, Smps);
    ECameraProjectionMode ProjectionMode = View->ProjectionMode;
    RHIDevice->SetAndUpdateConstantBuffer(PostProcessBufferType(View->NearClip, View->FarClip, ProjectionMode == ECameraProjectionMode::Orthographic));
   
    
    // 3) 셰이더
    UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>(UResourceManager::FullScreenVSPath);
    UShader* BlitPS = UResourceManager::GetInstance().Load<UShader>(UResourceManager::BlitPSPath);
    UShader* COCPS = UResourceManager::GetInstance().Load<UShader>(DOF_CalcCOC_PSPath);
    UShader* GaussianPS = UResourceManager::GetInstance().Load<UShader>(DOF_GaussianBlurPSPath);
    UShader* CompositePS = UResourceManager::GetInstance().Load<UShader>(DOF_CompositePSPath);

    //CalcCOC
    RHIDevice->PrepareShader(FullScreenTriangleVS, COCPS);
    URenderTexture* COCRT = RHIDevice->GetRenderTexture("DOF_COC");
    COCRT->InitResolution(RHIDevice, 1.0f);
    RHIDevice->OMSetRenderTargets(COCRT);
    TArray< ID3D11ShaderResourceView*> SRVs;
    SRVs.push_back(RHIDevice->GetSRV(RHI_SRV_Index::SceneDepth));
    SRVs.push_back(RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource));
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, SRVs.data());
    RHIDevice->DrawFullScreenQuad();

    //Blur
    URenderTexture* NearBlurRT = RHIDevice->GetRenderTexture("DOF_NearBlur");
    URenderTexture* FarBlurRT = RHIDevice->GetRenderTexture("DOF_FarBlur");
    NearBlurRT->InitResolution(RHIDevice, 1.0f);
    FarBlurRT->InitResolution(RHIDevice, 1.0f);
    SRVs.clear();
    SRVs.push_back(RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource));
    SRVs.push_back(COCRT->GetSRV());
    Gaussian(RHIDevice, SRVs.data(), NearBlurRT->GetRTV(), 2.0f, true);
    Gaussian(RHIDevice, SRVs.data(), FarBlurRT->GetRTV(), 4.0f, false);

    //Composite
    RHIDevice->PrepareShader(FullScreenTriangleVS, CompositePS);
    RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);
    SRVs.clear();
    SRVs.push_back(RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource));
    SRVs.push_back(COCRT->GetSRV());
    SRVs.push_back(NearBlurRT->GetSRV());
    SRVs.push_back(FarBlurRT->GetSRV());
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 4, SRVs.data());
    RHIDevice->DrawFullScreenQuad();

    // 8) 확정
    Swap.Commit();
}




void FDepthOfFieldPass::Gaussian(D3D11RHI* RHIDevice, ID3D11ShaderResourceView*const* SRVs, ID3D11RenderTargetView* RTV, float Weight, bool bNear)
{
    //Horizontal
    UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>(UResourceManager::FullScreenVSPath);
    UShader* GaussianPS = UResourceManager::GetInstance().Load<UShader>(DOF_GaussianBlurPSPath);

    RHIDevice->PrepareShader(FullScreenTriangleVS, GaussianPS);
    RHIDevice->SetAndUpdateConstantBuffer(FDOFGaussianCB(Weight, true, bNear));
    URenderTexture* GaussianHorizonRenderTexture = RHIDevice->GetRenderTexture("GaussianHorizon");
    GaussianHorizonRenderTexture->InitResolution(RHIDevice, 1.0f);
    RHIDevice->OMSetRenderTargets(GaussianHorizonRenderTexture);
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, SRVs);
    RHIDevice->DrawFullScreenQuad();

    //Vertical
    RHIDevice->SetAndUpdateConstantBuffer(FDOFGaussianCB(Weight, false, bNear));
    RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, &RTV, nullptr);
    TArray<ID3D11ShaderResourceView*> GaussianVerticalSRVs;
    GaussianVerticalSRVs.push_back(GaussianHorizonRenderTexture->GetSRV());
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, GaussianVerticalSRVs.data());
    RHIDevice->DrawFullScreenQuad();
}