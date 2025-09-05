#include "pch.h"
#include "Render/Public/Pipeline.h"

/// @brief 그래픽 파이프라인을 관리하는 클래스
UPipeline::UPipeline(ID3D11DeviceContext* DeviceContext) : DeviceContext(DeviceContext)
{
}

UPipeline::~UPipeline()
{
}


/// @brief 파이프라인 상태를 업데이트
void UPipeline::UpdatePipeline(FPipelineInfo Info)
{
	DeviceContext->IASetPrimitiveTopology(Info.topology);
	if (Info.InputLayout)
		DeviceContext->IASetInputLayout(Info.InputLayout);
	if (Info.VertexShader)
		DeviceContext->VSSetShader(Info.VertexShader, nullptr, 0);
	if (Info.RasterizerState)
		DeviceContext->RSSetState(Info.RasterizerState);
	if (Info.PixelShader)
		DeviceContext->PSSetShader(Info.PixelShader, nullptr, 0);
	if (Info.BlendState)
		DeviceContext->OMSetBlendState(Info.BlendState, nullptr, 0xffffffff);
}

/// @brief 정점 버퍼를 바인딩
void UPipeline::SetVertexBuffer(ID3D11Buffer* VertexBuffer, UINT Stride)
{
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
}

/// @brief 상수 버퍼를 설정
void UPipeline::SetConstantBuffer(UINT Slot, bool bIsVS, ID3D11Buffer* ConstantBuffer)
{
	if (ConstantBuffer)
	{
		if (bIsVS)
			DeviceContext->VSSetConstantBuffers(Slot, 1, &ConstantBuffer);
		else
			DeviceContext->PSSetConstantBuffers(Slot, 1, &ConstantBuffer);
	}
}

/// @brief 텍스처를 설정
void UPipeline::SetTexture(UINT Slot, bool bIsVS, ID3D11ShaderResourceView* Srv)
{
	if (Srv)
	{
		if (bIsVS)
			DeviceContext->VSSetShaderResources(Slot, 1, &Srv);
		else
			DeviceContext->PSSetShaderResources(Slot, 1, &Srv);
	}
}

/// @brief 샘플러 상태를 설정
void UPipeline::SetSamplerState(UINT Slot, bool bIsVS, ID3D11SamplerState* SamplerState)
{
	if (SamplerState)
	{
		if (bIsVS)
			DeviceContext->VSSetSamplers(Slot, 1, &SamplerState);
		else
			DeviceContext->PSSetSamplers(Slot, 1, &SamplerState);
	}
}

/// @brief 정점 개수를 기반으로 드로우 호출
void UPipeline::Draw(UINT VertexCount, UINT StartLocation)
{
	DeviceContext->Draw(VertexCount, StartLocation);
}
