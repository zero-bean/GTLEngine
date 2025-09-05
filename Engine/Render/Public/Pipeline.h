#pragma once

struct FPipelineInfo
{
	ID3D11InputLayout* InputLayout;
	ID3D11VertexShader* VertexShader;
	ID3D11RasterizerState* RasterizerState;
	ID3D11PixelShader* PixelShader;
	ID3D11BlendState* BlendState;
	D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};

class UPipeline
{
public:
	UPipeline(ID3D11DeviceContext* DeviceContext);
	~UPipeline();

	void UpdatePipeline(FPipelineInfo Info);

	void SetVertexBuffer(ID3D11Buffer* VertexBuffer, UINT Stride);

	void SetConstantBuffer(UINT Slot, bool bIsVS, ID3D11Buffer* ConstantBuffer);

	void SetTexture(UINT Slot, bool bIsVS, ID3D11ShaderResourceView* Srv);

	void SetSamplerState(UINT Slot, bool bIsVS, ID3D11SamplerState* SamplerState);

	void Draw(UINT VertexCount, UINT StartLocation);

private:
	ID3D11DeviceContext* DeviceContext;
};
