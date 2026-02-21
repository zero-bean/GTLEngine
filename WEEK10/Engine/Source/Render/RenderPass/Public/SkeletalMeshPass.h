#pragma once

#include "RenderPass.h"

class FSkeletalMeshPass : public FRenderPass
{
public:
	FSkeletalMeshPass(UPipeline* InPipeline,
		ID3D11Buffer* InConstantBufferViewProj,
		ID3D11Buffer* InConstantBufferModel,
		ID3D11VertexShader* InVS,
		ID3D11PixelShader* InPS,
		ID3D11InputLayout* InLayout,
		ID3D11DepthStencilState* InDS
	);

	virtual void SetRenderTargets(class UDeviceResources* DeviceResources) override;
	virtual void Execute(FRenderingContext& Context) override;
	virtual void Release() override;

	void SetVertexShader(ID3D11VertexShader* InVS) { VS = InVS; }
	void SetPixelShader(ID3D11PixelShader* InPS) { PS = InPS; }
	void SetInputLayout(ID3D11InputLayout* InLayout) { InputLayout = InLayout; }

private:
	ID3D11VertexShader* VS;
	ID3D11PixelShader* PS;
	ID3D11InputLayout* InputLayout;
	ID3D11DepthStencilState* DS;

	ID3D11Buffer* ConstantBufferMaterial;
};
