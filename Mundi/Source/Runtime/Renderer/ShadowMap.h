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

	/**
	 * @brief 이 쉐도우 맵이 할당한 전체 GPU 메모리 크기를 반환합니다 (바이트).
	 * @return 할당된 메모리 사용량 (바이트)
	 */
	uint64_t GetAllocatedMemoryBytes() const;

	/**
	 * @brief 실제 사용 중인 슬롯에 해당하는 메모리 크기를 반환합니다 (바이트).
	 * @param UsedSlotCount - 실제 사용 중인 슬롯 수
	 * @return 사용 중인 메모리 크기 (바이트)
	 */
	uint64_t GetUsedMemoryBytes(uint32 UsedSlotCount) const;

	/**
	 * @brief DXGI 포맷의 바이트 크기를 반환합니다 (픽셀당).
	 * @param format - DXGI 포맷
	 * @return 픽셀당 바이트 수
	 */
	static uint64_t GetDepthFormatSize(DXGI_FORMAT format);

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
