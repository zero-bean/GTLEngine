#pragma once
#include "pch.h"

struct FTextureRenderProxy
{
public:
	FTextureRenderProxy(ComPtr<ID3D11ShaderResourceView> InSRV, ComPtr<ID3D11SamplerState> InSampler)
		: SRV(std::move(InSRV)), Sampler(std::move(InSampler)) {}

	ID3D11ShaderResourceView* GetSRV() const { return SRV.Get(); }
	ID3D11SamplerState* GetSampler() const { return Sampler.Get(); }
	void Release() const { SRV->Release(); Sampler->Release(); }

private:
	ComPtr<ID3D11ShaderResourceView> SRV;
	ComPtr<ID3D11SamplerState> Sampler;
};
