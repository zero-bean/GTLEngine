#include "pch.h"
#include "ShadowMap.h"

FShadowMap::FShadowMap()
	: Width(0)
	, Height(0)
	, ArraySize(0)
	, bIsCubeMap(false)
	, FilterType(EShadowFilterType::NONE)
	, ShadowMapTexture(nullptr)
	, ShadowMapSRV(nullptr)
{
	ZeroMemory(&ShadowViewport, sizeof(D3D11_VIEWPORT));
}

FShadowMap::~FShadowMap()
{
	Release();
}

void FShadowMap::Initialize(D3D11RHI* RHI, UINT InWidth, UINT InHeight, UINT InArraySize, bool bInIsCubeMap, EShadowFilterType InFilterType)
{
	Width = InWidth;
	Height = InHeight;
	ArraySize = bInIsCubeMap ? InArraySize * 6 : InArraySize;
	bIsCubeMap = bInIsCubeMap;
	FilterType = InFilterType;

	// 필터 타입에 따라 텍스처 포맷 및 바인드 플래그 결정
	DXGI_FORMAT textureFormat;
	UINT bindFlags;

	if (FilterType == EShadowFilterType::NONE || FilterType == EShadowFilterType::PCF)
	{
		// Depth-only 방식 (기존 방식)
		textureFormat = DXGI_FORMAT_R32_TYPELESS;
		bindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	}
	else if (FilterType == EShadowFilterType::VSM || FilterType == EShadowFilterType::EVSM)
	{
		// VSM/EVSM: depth + depth^2 저장 (R32G32_FLOAT)
		textureFormat = DXGI_FORMAT_R32G32_FLOAT;
		bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	}
	else // ESM
	{
		// ESM: exponential depth 저장 (R32_FLOAT)
		textureFormat = DXGI_FORMAT_R32_FLOAT;
		bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	}

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = Width;
	texDesc.Height = Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = ArraySize;
	texDesc.Format = textureFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = bindFlags;
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

	// 각 배열 슬라이스에 대한 뷰 생성 (DSV 또는 RTV)
	ShadowMapDSVs.clear();
	ShadowMapRTVs.clear();

	if (FilterType == EShadowFilterType::NONE || FilterType == EShadowFilterType::PCF)
	{
		// Depth-only: DSV 생성
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
	}
	else
	{
		// VSM/ESM/EVSM: RTV 생성
		UINT numRTVs = ArraySize;
		for (UINT i = 0; i < numRTVs; i++)
		{
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = textureFormat; // R32G32_FLOAT 또는 R32_FLOAT
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.FirstArraySlice = i;
			rtvDesc.Texture2DArray.ArraySize = 1;

			ID3D11RenderTargetView* RTV = nullptr;
			hr = RHI->GetDevice()->CreateRenderTargetView(ShadowMapTexture, &rtvDesc, &RTV);
			assert(SUCCEEDED(hr), "Failed to create shadow map RTV");

			ShadowMapRTVs.Add(RTV);
		}
	}

	// 전체 배열에 대한 셰이더 리소스 뷰 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	// 필터 타입에 따라 SRV 포맷 결정
	if (FilterType == EShadowFilterType::NONE || FilterType == EShadowFilterType::PCF)
	{
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // Depth를 float로 읽기
	}
	else if (FilterType == EShadowFilterType::VSM || FilterType == EShadowFilterType::EVSM)
	{
		srvDesc.Format = DXGI_FORMAT_R32G32_FLOAT; // depth + depth^2
	}
	else // ESM
	{
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // exponential depth
	}

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

	// 개별 슬라이스 SRV 생성 (ImGui 표시용 - TEXTURE2DARRAY, ArraySize=1)
	ShadowMapSliceSRVs.clear();
	for (UINT i = 0; i < ArraySize; i++)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC sliceSrvDesc = {};
		sliceSrvDesc.Format = srvDesc.Format; // 동일한 포맷 사용
		sliceSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		sliceSrvDesc.Texture2DArray.MostDetailedMip = 0;
		sliceSrvDesc.Texture2DArray.MipLevels = 1;
		sliceSrvDesc.Texture2DArray.FirstArraySlice = i;
		sliceSrvDesc.Texture2DArray.ArraySize = 1; // 단일 슬라이스만

		ID3D11ShaderResourceView* SliceSRV = nullptr;
		hr = RHI->GetDevice()->CreateShaderResourceView(ShadowMapTexture, &sliceSrvDesc, &SliceSRV);
		assert(SUCCEEDED(hr), "Failed to create shadow map slice SRV");

		ShadowMapSliceSRVs.Add(SliceSRV);
	}

	// 뷰포트 설정
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

	for (ID3D11RenderTargetView* RTV : ShadowMapRTVs)
	{
		if (RTV)
		{
			RTV->Release();
			RTV = nullptr;
		}
	}
	ShadowMapRTVs.clear();

	if (ShadowMapTexture)
	{
		ShadowMapTexture->Release();
		ShadowMapTexture = nullptr;
	}

	// 캐시된 RasterizerState 해제
	for (auto& Pair : CachedRasterizerStates)
	{
		if (Pair.second)
		{
			Pair.second->Release();
		}
	}
	CachedRasterizerStates.clear();
}

void FShadowMap::BeginRender(D3D11RHI* RHI, UINT ArrayIndex, float DepthBias, float SlopeScaledDepthBias)
{
	assert(RHI != nullptr, "RHI is null");
	assert(ArrayIndex < ArraySize, "Array index out of bounds");

	ID3D11DeviceContext* pContext = RHI->GetDeviceContext();

	// 섀도우 맵 SRV 언바인딩 (리소스 hazard 방지: t5=SpotLight, t6=DirectionalLight, t7=PointLight)
	ID3D11ShaderResourceView* pNullSRVs[3] = { nullptr, nullptr, nullptr };
	pContext->PSSetShaderResources(5, 3, pNullSRVs);

	// RasterizerState 캐싱 (DepthBias 조합별로 재사용)
	FRasterizerStateKey Key = { DepthBias, SlopeScaledDepthBias };
	ID3D11RasterizerState* RasterizerState = nullptr;

	// 캐시에서 찾기
	auto it = CachedRasterizerStates.find(Key);
	if (it != CachedRasterizerStates.end())
	{
		// 캐시에 있으면 재사용
		RasterizerState = it->second;
	}
	else
	{
		// 캐시에 없으면 새로 생성하고 캐시에 저장
		D3D11_RASTERIZER_DESC ShadowRasterizerDesc = {};
		ShadowRasterizerDesc.FillMode = D3D11_FILL_SOLID;
		ShadowRasterizerDesc.CullMode = D3D11_CULL_BACK;
		ShadowRasterizerDesc.DepthClipEnable = TRUE;
		ShadowRasterizerDesc.DepthBias = static_cast<INT>(DepthBias * 100000.0f);
		ShadowRasterizerDesc.SlopeScaledDepthBias = SlopeScaledDepthBias;
		ShadowRasterizerDesc.DepthBiasClamp = 0.0f;

		RHI->GetDevice()->CreateRasterizerState(&ShadowRasterizerDesc, &RasterizerState);
		CachedRasterizerStates[Key] = RasterizerState;
	}

	pContext->RSSetState(RasterizerState);

	// 필터 타입에 따라 다른 렌더링 경로 사용
	if (FilterType == EShadowFilterType::NONE || FilterType == EShadowFilterType::PCF)
	{
		// Depth-only 렌더링 (기존 방식)
		ID3D11DepthStencilView* DSV = ShadowMapDSVs[ArrayIndex];
		pContext->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Pixel shader unbind (depth-only)
		pContext->PSSetShader(nullptr, nullptr, 0);

		// Depth-stencil state 설정
		RHI->OMSetDepthStencilState(EComparisonFunc::LessEqual);

		// OM: 컬러 렌더링 없이 깊이만 쓰기
		ID3D11RenderTargetView* nullRTV = nullptr;
		pContext->OMSetRenderTargets(1, &nullRTV, DSV);
	}
	else
	{
		// VSM/ESM/EVSM: RTV에 depth 정보 렌더링 (픽셀 쉐이더 필요)
		ID3D11RenderTargetView* RTV = ShadowMapRTVs[ArrayIndex];

		// RTV 유효성 검사
		if (!RTV)
		{
			UE_LOG("BeginRender: RTV is null for ArrayIndex %d (FilterType=%d)", ArrayIndex, (int)FilterType);
			return;
		}

		// RTV 클리어 (white = 무한대 depth를 의미)
		float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		pContext->ClearRenderTargetView(RTV, clearColor);

		// VSM/ESM/EVSM용 픽셀 쉐이더는 SceneRenderer::OverrideShadowShader()에서 설정됨

		// Depth-stencil state 비활성화 (depth test/write 불필요)
		RHI->OMSetDepthStencilState(EComparisonFunc::Always);

		// OM: RTV만 바인딩 (DSV 없음)
		pContext->OMSetRenderTargets(1, &RTV, nullptr);
	}

	// 뷰포트 설정
	D3D11_VIEWPORT ScaledViewport = ShadowViewport;
	ScaledViewport.Width = ShadowViewport.Width;
	ScaledViewport.Height = ShadowViewport.Height;
	pContext->RSSetViewports(1, &ScaledViewport);
}

void FShadowMap::EndRender(D3D11RHI* RHI)
{
	assert(RHI != nullptr, "RHI is null");

	// Unbind render targets
	ID3D11RenderTargetView* nullRTV = nullptr;
	ID3D11DepthStencilView* nullDSV = nullptr;
	RHI->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullDSV);
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
		// 64-bit formats (8 bytes per pixel) - VSM/EVSM
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return 8;

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
