#pragma once

class FShadowMap
{
public:
	FShadowMap();
	~FShadowMap();

	// 섀도우맵 배열 초기화 (해상도 및 개수 지정)
	// InWidth - 섀도우맵 너비 (기본값 1024)
	// InHeight - 섀도우맵 높이 (기본값 1024)
	// InArraySize - 배열 내 섀도우맵 개수 (기본값 4)
	// bInIsCubeMap - true면 TextureCubeArray, false면 Texture2DArray (기본값 false)
	void Initialize(D3D11RHI* RHI, UINT InWidth = 1024, UINT InHeight = 1024, UINT InArraySize = 4, bool bInIsCubeMap = false);
	void Release();

	// 특정 배열 슬라이스에 렌더링 시작
	// RHI - D3D11 디바이스 인터페이스
	// ArrayIndex - 렌더링할 배열 슬라이스 인덱스 (CubeMap인 경우 CubeIndex * 6 + FaceIndex)
	// DepthBias - 섀도우 뎁스 바이어스 (기본값 10)
	// SlopeScaledDepthBias - 섀도우 슬로프 바이어스 (기본값 1.0f)
	void BeginRender(D3D11RHI* RHI, UINT ArrayIndex, float DepthBias = 10.0f, float SlopeScaledDepthBias = 1.0f);
	void EndRender(D3D11RHI* RHI);

	ID3D11ShaderResourceView* GetSRV() const { return ShadowMapSRV; }
	ID3D11ShaderResourceView* GetSliceSRV(UINT ArrayIndex) const
	{
		return (ArrayIndex < ShadowMapSliceSRVs.size()) ? ShadowMapSliceSRVs[ArrayIndex] : nullptr;
	}
	UINT GetWidth() const { return Width; }
	UINT GetHeight() const { return Height; }
	UINT GetArraySize() const { return ArraySize; }
	bool IsCubeMap() const { return bIsCubeMap; }
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

	/**
	 * @brief 특정 슬라이스의 깊이 값을 MinDepth~MaxDepth 범위로 리매핑하여 시각화용 SRV를 반환합니다.
	 * @param RHI - D3D11 RHI 디바이스
	 * @param ArrayIndex - 렌더링할 배열 슬라이스 인덱스
	 * @param MinDepth - 리매핑 시작 범위 (예: 0.99)
	 * @param MaxDepth - 리매핑 끝 범위 (예: 1.0)
	 * @return 리매핑된 텍스처의 SRV (시각화 전용, 원본 데이터는 변경되지 않음)
	 */
	ID3D11ShaderResourceView* GetRemappedSliceSRV(D3D11RHI* RHI, UINT ArrayIndex, float MinDepth, float MaxDepth);

private:
	// 깊이 리매핑 초기화 (DepthRemap.hlsl 셰이더 컴파일 및 리소스 생성)
	void InitializeDepthRemapResources(D3D11RHI* RHI);
	void ReleaseDepthRemapResources();
	// 섀도우맵 크기
	UINT Width;
	UINT Height;
	UINT ArraySize;

	// 큐브맵 여부
	bool bIsCubeMap;

	// 깊이 텍스처 배열 리소스
	ID3D11Texture2D* ShadowMapTexture;
	ID3D11ShaderResourceView* ShadowMapSRV; // Full array SRV (for shaders)
	TArray<ID3D11ShaderResourceView*> ShadowMapSliceSRVs; // Individual slice SRVs (for ImGui)

	// PointLight, DirectionalLight는 여러 맵을 필요로 하기에 확장성을 위해 배열로 보유합니다.
	TArray<ID3D11DepthStencilView*> ShadowMapDSVs;

	// 섀도우 렌더링용 뷰포트
	D3D11_VIEWPORT ShadowViewport;

	// 깊이 리매핑 시각화 리소스 (Editor용)
	ID3D11Texture2D* RemappedTexture;
	ID3D11RenderTargetView* RemappedRTV;
	ID3D11ShaderResourceView* RemappedSRV;
	ID3D11VertexShader* DepthRemapVS;
	ID3D11PixelShader* DepthRemapPS;
	ID3D11Buffer* DepthRemapConstantBuffer;
	ID3D11Buffer* FullscreenQuadVB;
	ID3D11InputLayout* DepthRemapInputLayout;
	ID3D11SamplerState* PointSamplerState;
};
