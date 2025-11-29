#pragma once
#include  "PostProcessing.h"

class FSceneView;
class FDepthOfFieldPass final : public IPostProcessPass
{
public:
    static const char* DOF_CompositePSPath;
    static const char* DOF_GaussianBlurPSPath;
    static const char* DOF_CalcCOC_PSPath;
public:
    virtual void Execute(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice) override;

private:
    void Gaussian(D3D11RHI* RHIDevice, ID3D11ShaderResourceView* const* SRVs, ID3D11RenderTargetView* RTV, float Weight, bool bNear);
};
