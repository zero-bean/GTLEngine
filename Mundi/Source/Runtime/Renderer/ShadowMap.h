#pragma once

class FShadowMap
{
public:
	FShadowMap();
	~FShadowMap();

	/**
	 * Initialize shadow map array with specified resolution and count
	 * @param InWidth - Shadow map width (default 1024)
	 * @param InHeight - Shadow map height (default 1024)
	 * @param InArraySize - Number of shadow maps in array (default 4)
	 */
	void Initialize(D3D11RHI* RHI, UINT InWidth = 1024, UINT InHeight = 1024, UINT InArraySize = 4);
	void Release();

	/**
	 * Begin rendering to a specific array slice
	 * @param RHI - D3D11 device interface
	 * @param ArrayIndex - Index of the array slice to render to
	 */
	void BeginRender(D3D11RHI* RHI, UINT ArrayIndex);
	void EndRender(D3D11RHI* RHI);

	ID3D11ShaderResourceView* GetSRV() const { return ShadowMapSRV; }
	UINT GetWidth() const { return Width; }
	UINT GetHeight() const { return Height; }
	UINT GetArraySize() const { return ArraySize; }
	const D3D11_VIEWPORT& GetViewport() const { return ShadowViewport; }

private:
	// Shadow map dimensions
	UINT Width;
	UINT Height;
	UINT ArraySize;

	// Depth texture array resources
	ID3D11Texture2D* ShadowMapTexture;
	ID3D11ShaderResourceView* ShadowMapSRV;

	// PointLight, DirectionalLight는 여러 맵을 필요로 하기에 확장성을 위해 배열로 보유합니다.
	TArray<ID3D11DepthStencilView*> ShadowMapDSVs;

	// Viewport for shadow rendering
	D3D11_VIEWPORT ShadowViewport;
};
