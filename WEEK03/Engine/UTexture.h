#pragma once
#include "UObject.h"

class UTexture : public UObject
{
	DECLARE_UCLASS(UTexture, UObject)
public:
	UTexture() = default;
	virtual ~UTexture() override;

	bool LoadFromFile(ID3D11Device* InDevice, const std::wstring& InFilePath);

	void Bind(ID3D11DeviceContext* InContext, const UINT InSlot = 0);

	// Getter //
	ID3D11ShaderResourceView* GetSRV() const { return ShaderResourceView; }
	ID3D11SamplerState* GetSamplerState() const { return SamplerState; }
	// Setter //

private:
	/* If you need, then extend of this function */
	bool CreateSamplerState(ID3D11Device* InDevice);

	ID3D11ShaderResourceView* ShaderResourceView{ nullptr };
	ID3D11SamplerState* SamplerState{ nullptr };
};
