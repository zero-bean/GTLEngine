#pragma once
#include "RenderPass.h"

class FCameraPostProcessPass : public FRenderPass
{
public:
	FCameraPostProcessPass(UPipeline* InPipeline, ID3D11DepthStencilState* InDS, ID3D11BlendState* InBS);

	void SetRenderTargets(UDeviceResources* DeviceResources) override;
	void Execute(FRenderingContext& Context) override;
	void Release() override;

private:
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11BlendState* BS;
	ID3D11Buffer* CBRenderTarget;
	ID3D11Buffer* CBFade;
	ID3D11DepthStencilState* DS;
};
