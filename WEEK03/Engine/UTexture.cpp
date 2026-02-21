#include "stdafx.h"
#include "UTexture.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UTexture, UObject)

UTexture::~UTexture()
{
	SAFE_RELEASE(ShaderResourceView);
	SAFE_RELEASE(SamplerState);
}

bool UTexture::LoadFromFile(ID3D11Device* InDevice, const std::wstring& InFilePath)
{
	if (FAILED(DirectX::CreateDDSTextureFromFile(InDevice, InFilePath.c_str(), nullptr, &ShaderResourceView)))
	{
		OutputDebugStringA("UTexture::LoadFromFile, failed to create texture from file");
		return false;
	}

	if (CreateSamplerState(InDevice) == false)
	{
		OutputDebugStringA("UTexture::LoadFromFile, failed to create sampler state");
		return false;
	}

	return true;
}

void UTexture::Bind(ID3D11DeviceContext* InContext, const UINT InSlot)
{
	if (ShaderResourceView) { InContext->PSSetShaderResources(InSlot, 1, &ShaderResourceView); }
	if (SamplerState) { InContext->PSSetSamplers(InSlot, 1, &SamplerState); }
}

/* *
* D3D11_SAMPLER_DESC
* https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ns-d3d11-d3d11_sampler_desc
*/
bool UTexture::CreateSamplerState(ID3D11Device* InDevice)
{
	D3D11_SAMPLER_DESC SamplerDesc{};
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	return SUCCEEDED(InDevice->CreateSamplerState(&SamplerDesc, &SamplerState));
}
