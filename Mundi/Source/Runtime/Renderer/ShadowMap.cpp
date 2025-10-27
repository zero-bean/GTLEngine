#include "pch.h"
#include "ShadowMap.h"

FShadowMap::FShadowMap()
	: Width(0)
	, Height(0)
	, ArraySize(0)
	, bIsCubeMap(false)
	, ShadowMapTexture(nullptr)
	, ShadowMapSRV(nullptr)
{
	ZeroMemory(&ShadowViewport, sizeof(D3D11_VIEWPORT));
}

FShadowMap::~FShadowMap()
{
	Release();
}

void FShadowMap::Initialize(D3D11RHI* RHI, UINT InWidth, UINT InHeight, UINT InArraySize, bool bInIsCubeMap)
{
	Width = InWidth;
	Height = InHeight;
	ArraySize = bInIsCubeMap ? InArraySize * 6 : InArraySize;
	bIsCubeMap = bInIsCubeMap;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = Width;
	texDesc.Height = Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = ArraySize;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Typeless를 선언해야만, 텍스쳐 Read & Write 2가지가 가능해집니다.
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // DSV 및 SRV를 사용하도록 설정합니다.
	texDesc.CPUAccessFlags = 0;

	// CubeMap인 경우 TEXTURECUBE 플래그 추가
	if (bIsCubeMap)
	{
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	}
	else
	{
		texDesc.MiscFlags = 0;
	}

	HRESULT hr = RHI->GetDevice()->CreateTexture2D(&texDesc, nullptr, &ShadowMapTexture);
	assert(SUCCEEDED(hr), "Failed to create shadow map texture array");

	// 각 배열 슬라이스에 대한 깊이 스텐실 뷰 생성
	ShadowMapDSVs.clear();
	UINT numDSVs = ArraySize;
	for (UINT i = 0; i < numDSVs; i++)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 32-bit depth
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = i;
		dsvDesc.Texture2DArray.ArraySize = 1;
		dsvDesc.Flags = 0;

		ID3D11DepthStencilView* DSV = nullptr;
		hr = RHI->GetDevice()->CreateDepthStencilView(ShadowMapTexture, &dsvDesc, &DSV);
		assert(SUCCEEDED(hr), "Failed to create shadow map DSV");

		ShadowMapDSVs.Add(DSV);
	}

	// 전체 배열에 대한 셰이더 리소스 뷰 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // 셰이더에서 float로 읽기

	if (bIsCubeMap)
	{
		// TextureCubeArray로 바인딩
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
		srvDesc.TextureCubeArray.MostDetailedMip = 0;
		srvDesc.TextureCubeArray.MipLevels = 1;
		srvDesc.TextureCubeArray.First2DArrayFace = 0;
		srvDesc.TextureCubeArray.NumCubes = ArraySize / 6; // 큐브 개수
	}
	else
	{
		// Texture2DArray로 바인딩
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = ArraySize;
	}

	hr = RHI->GetDevice()->CreateShaderResourceView(ShadowMapTexture, &srvDesc, &ShadowMapSRV);
	assert(SUCCEEDED(hr), "Failed to create shadow map SRV");

	// Create individual slice SRVs (for ImGui - TEXTURE2D not array)
	ShadowMapSliceSRVs.clear();
	for (UINT i = 0; i < ArraySize; i++)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC sliceSrvDesc = {};
		sliceSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		sliceSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		sliceSrvDesc.Texture2DArray.MostDetailedMip = 0;
		sliceSrvDesc.Texture2DArray.MipLevels = 1;
		sliceSrvDesc.Texture2DArray.FirstArraySlice = i;
		sliceSrvDesc.Texture2DArray.ArraySize = 1; // Single slice only

		ID3D11ShaderResourceView* SliceSRV = nullptr;
		hr = RHI->GetDevice()->CreateShaderResourceView(ShadowMapTexture, &sliceSrvDesc, &SliceSRV);
		assert(SUCCEEDED(hr), "Failed to create shadow map slice SRV");

		ShadowMapSliceSRVs.Add(SliceSRV);
	}

	// Setup viewport
	ShadowViewport.TopLeftX = 0.0f;
	ShadowViewport.TopLeftY = 0.0f;
	ShadowViewport.Width = static_cast<float>(Width);
	ShadowViewport.Height = static_cast<float>(Height);
	ShadowViewport.MinDepth = 0.0f;
	ShadowViewport.MaxDepth = 1.0f;
}

void FShadowMap::Release()
{
	if (ShadowMapSRV)
	{
		ShadowMapSRV->Release();
		ShadowMapSRV = nullptr;
	}

	for (ID3D11ShaderResourceView* SliceSRV : ShadowMapSliceSRVs)
	{
		if (SliceSRV)
		{
			SliceSRV->Release();
			SliceSRV = nullptr;
		}
	}
	ShadowMapSliceSRVs.clear();

	for (ID3D11DepthStencilView* DSV : ShadowMapDSVs)
	{
		if (DSV)
		{
			DSV->Release();
			DSV = nullptr;
		}
	}
	ShadowMapDSVs.clear();

	if (ShadowMapTexture)
	{
		ShadowMapTexture->Release();
		ShadowMapTexture = nullptr;
	}
}

void FShadowMap::BeginRender(D3D11RHI* RHI, UINT ArrayIndex)
{
	assert(RHI != nullptr, "RHI is null");
	assert(ArrayIndex < ArraySize, "Array index out of bounds");

	UE_LOG("[ShadowMap] BeginRender - ArrayIndex=%d, ArraySize=%d, Width=%d, Height=%d",
		ArrayIndex, ArraySize, Width, Height);

	ID3D11DeviceContext* pContext = RHI->GetDeviceContext();

	// N개의 쉐도우 DSV에서 특정 DSV를 가져옵니다.
	ID3D11DepthStencilView* DSV = ShadowMapDSVs[ArrayIndex];

	UE_LOG("[ShadowMap] BeginRender - DSV=0x%p, SRV=0x%p", DSV, ShadowMapSRV);

	// DSV를 초기화합니다.
	// REVERSE-Z: Clear to 0.0 (far plane) instead of 1.0
	pContext->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH, 0.0f, 0);

	// Unbind all pixel shader resources (depth-only pass doesn't need textures)
	ID3D11ShaderResourceView* pNullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { nullptr };
	pContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pNullSRVs);

	// Unbind pixel shader (depth-only rendering)
	pContext->PSSetShader(nullptr, nullptr, 0);

	// Set rasterizer state to Shadow (proper culling and depth bias for shadow acne prevention)
	RHI->RSSetState(ERasterizerMode::Shadow);

	// Set depth-stencil state for shadow rendering (depth write enabled, depth test enabled)
	// REVERSE-Z: Use GreaterEqual instead of LessEqual
	RHI->OMSetDepthStencilState(EComparisonFunc::GreaterEqual);

	// OM 단계에서, 컬러 렌더링을 하지 않고 오직 깊이 버퍼만 쓰도록 설정합니다.
	ID3D11RenderTargetView* nullRTV = nullptr;
	pContext->OMSetRenderTargets(1, &nullRTV, DSV);

	// Initialize 함수를 통해 갱신된 뷰포트 멤버 변수로 뷰포트를 설정합니다.
	pContext->RSSetViewports(1, &ShadowViewport);
}

void FShadowMap::EndRender(D3D11RHI* RHI)
{
	assert(RHI != nullptr, "RHI is null");

	UE_LOG("[ShadowMap] EndRender - Unbinding DSV, SRV=0x%p still valid", ShadowMapSRV);

	// Unbind render targets (DSV만 unbind, SRV는 유지)
	ID3D11RenderTargetView* nullRTV = nullptr;
	ID3D11DepthStencilView* nullDSV = nullptr;
	RHI->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullDSV);

	UE_LOG("[ShadowMap] EndRender - DSV unbound, SRV ready for read access");
}

uint64_t FShadowMap::GetAllocatedMemoryBytes() const
{
	// 텍스처가 초기화되지 않았으면 0 반환
	if (!ShadowMapTexture)
	{
		return 0;
	}

	// 텍스처 Description을 쿼리하여 실제 포맷 확인
	D3D11_TEXTURE2D_DESC desc;
	ShadowMapTexture->GetDesc(&desc);

	// 포맷별 바이트 크기 계산
	uint64_t BytesPerPixel = GetDepthFormatSize(desc.Format);

	// 전체 할당된 메모리 = (Width * Height) * BytesPerPixel * ArraySize
	uint64_t PixelsPerMap = static_cast<uint64_t>(Width) * static_cast<uint64_t>(Height);
	uint64_t TotalMemory = PixelsPerMap * BytesPerPixel * ArraySize;

	return TotalMemory;
}

uint64_t FShadowMap::GetUsedMemoryBytes(uint32 UsedSlotCount) const
{
	// 텍스처가 초기화되지 않았으면 0 반환
	if (!ShadowMapTexture)
	{
		return 0;
	}

	// 텍스처 Description을 쿼리하여 실제 포맷 확인
	D3D11_TEXTURE2D_DESC desc;
	ShadowMapTexture->GetDesc(&desc);

	// 포맷별 바이트 크기 계산
	uint64_t BytesPerPixel = GetDepthFormatSize(desc.Format);

	// 사용 중인 메모리 = (Width * Height) * BytesPerPixel * UsedSlotCount
	uint64_t PixelsPerMap = static_cast<uint64_t>(Width) * static_cast<uint64_t>(Height);
	uint64_t UsedMemory = PixelsPerMap * BytesPerPixel * UsedSlotCount;

	return UsedMemory;
}

uint64_t FShadowMap::GetDepthFormatSize(DXGI_FORMAT format)
{
	switch (format)
	{
		// 32-bit formats (4 bytes per pixel)
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return 4;

		// 16-bit formats (2 bytes per pixel)
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
			return 2;

		// 기본값: 32-bit (안전한 상한선)
		default:
			return 4;
	}
}
