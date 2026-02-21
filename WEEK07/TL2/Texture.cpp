#include "pch.h"
#include "Texture.h"
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h> // .png, .jpg 로더 추가
#include <filesystem> // 파일 확장자 확인을 위해 추가
#include <algorithm> // 확장자 소문자 변환을 위해 추가

UTexture::UTexture()
{
	Width = 0;
	Height = 0;
	Format = DXGI_FORMAT_UNKNOWN;
}

UTexture::~UTexture()
{
	ReleaseResources();
}

void UTexture::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
	assert(InDevice);

	std::wstring WFilePath(InFilePath.begin(), InFilePath.end());
	std::filesystem::path fsPath(WFilePath);
	FString extension = fsPath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	HRESULT hr = E_FAIL;

	if (extension == ".dds")
	{
		hr = DirectX::CreateDDSTextureFromFile(
			InDevice,
			WFilePath.c_str(),
			reinterpret_cast<ID3D11Resource**>(&Texture2D),
			&ShaderResourceView
		);
	}
	else // .png, .jpg, .bmp 등 다른 이미지 포맷 로드 시도
	{
		hr = DirectX::CreateWICTextureFromFile(
			InDevice,
			WFilePath.c_str(),
			reinterpret_cast<ID3D11Resource**>(&Texture2D),
			&ShaderResourceView
		);
	}

	if (FAILED(hr))
	{
		UE_LOG("!!! FAILED to load texture: %s", InFilePath.c_str());
	}
	else
	{
		if (Texture2D)
		{
			D3D11_TEXTURE2D_DESC desc;
			Texture2D->GetDesc(&desc);
			Width = desc.Width;
			Height = desc.Height;
			Format = desc.Format;
		}
		UE_LOG("Successfully loaded texture: %s", InFilePath.c_str());
	}
}

void UTexture::ReleaseResources()
{
	if(Texture2D)
	{
		Texture2D->Release();
	}

	if(ShaderResourceView)
	{
		ShaderResourceView->Release();
	}

	Width = 0;
	Height = 0;
	Format = DXGI_FORMAT_UNKNOWN;
}
