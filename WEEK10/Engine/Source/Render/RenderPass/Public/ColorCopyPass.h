#pragma once
#include "Render/RenderPass/Public/RenderPass.h"

class FColorCopyPass : FRenderPass
{
public:
	FColorCopyPass(UPipeline* InPipeline, ID3D11DepthStencilState* InDS);
	void SetRenderTargets(UDeviceResources* DeviceResources) override;
	void Execute(FRenderingContext& Context) override;
	void Release() override;

private:
	ID3D11SamplerState* SamplerState = nullptr;
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11ShaderResourceView* SceneSRV = nullptr;
	ID3D11DepthStencilState* DS;
	ID3D11Buffer* CBRenderTarget;
};
