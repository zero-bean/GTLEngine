#include "pch.h"
#include "ShadowMap.h"

FShadowMap::FShadowMap()
	: Width(0)
	, Height(0)
	, ArraySize(0)
	, bIsCubeMap(false)
	, ShadowMapTexture(nullptr)
	, ShadowMapSRV(nullptr)
	, RemappedTexture(nullptr)
	, RemappedRTV(nullptr)
	, RemappedSRV(nullptr)
	, DepthRemapVS(nullptr)
	, DepthRemapPS(nullptr)
	, DepthRemapConstantBuffer(nullptr)
	, FullscreenQuadVB(nullptr)
	, DepthRemapInputLayout(nullptr)
	, PointSamplerState(nullptr)
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

	// 개별 슬라이스 SRV 생성 (ImGui 표시용 - TEXTURE2DARRAY, ArraySize=1)
	ShadowMapSliceSRVs.clear();
	for (UINT i = 0; i < ArraySize; i++)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC sliceSrvDesc = {};
		sliceSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
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

	// Release depth remap visualization resources
	ReleaseDepthRemapResources();
}

void FShadowMap::BeginRender(D3D11RHI* RHI, UINT ArrayIndex, float DepthBias, float SlopeScaledDepthBias)
{
	assert(RHI != nullptr, "RHI is null");
	assert(ArrayIndex < ArraySize, "Array index out of bounds");

	ID3D11DeviceContext* pContext = RHI->GetDeviceContext();

	// N개의 쉐도우 DSV에서 특정 DSV를 가져옵니다.
	ID3D11DepthStencilView* DSV = ShadowMapDSVs[ArrayIndex];

	// DSV를 초기화합니다.
	pContext->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// 섀도우 맵 SRV 언바인딩 (리소스 hazard 방지: t5=SpotLight, t6=DirectionalLight, t7=PointLight)
	ID3D11ShaderResourceView* pNullSRVs[3] = { nullptr, nullptr, nullptr };
	pContext->PSSetShaderResources(5, 3, pNullSRVs);

	// Unbind pixel shader (depth-only rendering)
	pContext->PSSetShader(nullptr, nullptr, 0);

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

	// Set depth-stencil state for shadow rendering (depth write enabled, depth test enabled)
	RHI->OMSetDepthStencilState(EComparisonFunc::LessEqual);

	// OM 단계에서, 컬러 렌더링을 하지 않고 오직 깊이 버퍼만 쓰도록 설정합니다.
	ID3D11RenderTargetView* nullRTV = nullptr;
	pContext->OMSetRenderTargets(1, &nullRTV, DSV);

	// 해상도 스케일을 적용한 뷰포트 설정
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

void FShadowMap::ReleaseDepthRemapResources()
{
	if (PointSamplerState)
	{
		PointSamplerState->Release();
		PointSamplerState = nullptr;
	}

	if (DepthRemapInputLayout)
	{
		DepthRemapInputLayout->Release();
		DepthRemapInputLayout = nullptr;
	}

	if (FullscreenQuadVB)
	{
		FullscreenQuadVB->Release();
		FullscreenQuadVB = nullptr;
	}

	if (DepthRemapConstantBuffer)
	{
		DepthRemapConstantBuffer->Release();
		DepthRemapConstantBuffer = nullptr;
	}

	if (DepthRemapPS)
	{
		DepthRemapPS->Release();
		DepthRemapPS = nullptr;
	}

	if (DepthRemapVS)
	{
		DepthRemapVS->Release();
		DepthRemapVS = nullptr;
	}

	if (RemappedSRV)
	{
		RemappedSRV->Release();
		RemappedSRV = nullptr;
	}

	if (RemappedRTV)
	{
		RemappedRTV->Release();
		RemappedRTV = nullptr;
	}

	if (RemappedTexture)
	{
		RemappedTexture->Release();
		RemappedTexture = nullptr;
	}
}

void FShadowMap::InitializeDepthRemapResources(D3D11RHI* RHI)
{
	// 이미 초기화되었으면 스킵
	if (DepthRemapVS != nullptr)
	{
		return;
	}

	ID3D11Device* device = RHI->GetDevice();
	HRESULT hr;

	// 1. Compile shaders from DepthRemap.hlsl
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// Vertex Shader
	hr = D3DCompileFromFile(
		L"Shaders/Utility/DepthRemap.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VS",
		"vs_5_0",
		compileFlags,
		0,
		&vsBlob,
		&errorBlob);

	if (FAILED(hr))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		assert(false, "Failed to compile DepthRemap vertex shader");
		return;
	}

	hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &DepthRemapVS);
	assert(SUCCEEDED(hr), "Failed to create DepthRemap vertex shader");

	// Pixel Shader
	hr = D3DCompileFromFile(
		L"Shaders/Utility/DepthRemap.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PS",
		"ps_5_0",
		compileFlags,
		0,
		&psBlob,
		&errorBlob);

	if (FAILED(hr))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		assert(false, "Failed to compile DepthRemap pixel shader");
		vsBlob->Release();
		return;
	}

	hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &DepthRemapPS);
	assert(SUCCEEDED(hr), "Failed to create DepthRemap pixel shader");

	// 2. Create Input Layout
	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = device->CreateInputLayout(inputDesc, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &DepthRemapInputLayout);
	assert(SUCCEEDED(hr), "Failed to create DepthRemap input layout");

	vsBlob->Release();
	psBlob->Release();

	// 3. Create Fullscreen Quad Vertex Buffer
	struct Vertex
	{
		float Position[3];
		float TexCoord[2];
	};

	// 시계방향 90도 회전된 텍스처 좌표
	Vertex quadVertices[] = {
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f } }, // Top-left
		{ {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f } }, // Top-right
		{ { -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } }, // Bottom-left
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } }  // Bottom-right
	};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = sizeof(quadVertices);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = quadVertices;

	hr = device->CreateBuffer(&vbDesc, &vbData, &FullscreenQuadVB);
	assert(SUCCEEDED(hr), "Failed to create fullscreen quad vertex buffer");

	// 4. Create Constant Buffer
	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth = 16; // float MinDepth, float MaxDepth, float2 Padding
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = device->CreateBuffer(&cbDesc, nullptr, &DepthRemapConstantBuffer);
	assert(SUCCEEDED(hr), "Failed to create DepthRemap constant buffer");

	// 5. Create Point Sampler
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = device->CreateSamplerState(&samplerDesc, &PointSamplerState);
	assert(SUCCEEDED(hr), "Failed to create point sampler state");

	// 6. Create Remapped Render Target
	D3D11_TEXTURE2D_DESC rtDesc = {};
	rtDesc.Width = Width;
	rtDesc.Height = Height;
	rtDesc.MipLevels = 1;
	rtDesc.ArraySize = 1;
	rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtDesc.SampleDesc.Count = 1;
	rtDesc.SampleDesc.Quality = 0;
	rtDesc.Usage = D3D11_USAGE_DEFAULT;
	rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	rtDesc.CPUAccessFlags = 0;
	rtDesc.MiscFlags = 0;

	hr = device->CreateTexture2D(&rtDesc, nullptr, &RemappedTexture);
	assert(SUCCEEDED(hr), "Failed to create remapped render target texture");

	hr = device->CreateRenderTargetView(RemappedTexture, nullptr, &RemappedRTV);
	assert(SUCCEEDED(hr), "Failed to create remapped render target view");

	hr = device->CreateShaderResourceView(RemappedTexture, nullptr, &RemappedSRV);
	assert(SUCCEEDED(hr), "Failed to create remapped shader resource view");
}

ID3D11ShaderResourceView* FShadowMap::GetRemappedSliceSRV(D3D11RHI* RHI, UINT ArrayIndex, float MinDepth, float MaxDepth)
{
	// 초기화되지 않았거나 인덱스가 범위를 벗어나면 nullptr 반환
	if (ArrayIndex >= ArraySize || !ShadowMapTexture)
	{
		return nullptr;
	}

	// Depth remap 리소스 초기화 (최초 1회만)
	InitializeDepthRemapResources(RHI);

	ID3D11DeviceContext* context = RHI->GetDeviceContext();

	// 0. Save current render state
	ID3D11RenderTargetView* oldRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };
	ID3D11DepthStencilView* oldDSV = nullptr;
	context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, oldRTVs, &oldDSV);

	UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	D3D11_VIEWPORT oldViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	context->RSGetViewports(&numViewports, oldViewports);

	// 1. Update Constant Buffer
	struct DepthRemapParams
	{
		float MinDepth;
		float MaxDepth;
		float Padding[2];
	};

	DepthRemapParams params = { MinDepth, MaxDepth, { 0.0f, 0.0f } };

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = context->Map(DepthRemapConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (SUCCEEDED(hr))
	{
		memcpy(mappedResource.pData, &params, sizeof(DepthRemapParams));
		context->Unmap(DepthRemapConstantBuffer, 0);
	}

	// 2. Set Render Target
	context->OMSetRenderTargets(1, &RemappedRTV, nullptr);

	// 3. Set Viewport
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(Width);
	viewport.Height = static_cast<float>(Height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);

	// 4. Set Shaders and Resources
	context->VSSetShader(DepthRemapVS, nullptr, 0);
	context->PSSetShader(DepthRemapPS, nullptr, 0);
	context->IASetInputLayout(DepthRemapInputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// 5. Bind Source Depth Texture (specific slice)
	ID3D11ShaderResourceView* sourceSRV = GetSliceSRV(ArrayIndex);
	context->PSSetShaderResources(0, 1, &sourceSRV);
	context->PSSetSamplers(0, 1, &PointSamplerState);
	context->VSSetConstantBuffers(0, 1, &DepthRemapConstantBuffer);
	context->PSSetConstantBuffers(0, 1, &DepthRemapConstantBuffer);

	// 6. Draw Fullscreen Quad
	UINT stride = sizeof(float) * 5; // 3 pos + 2 uv
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &FullscreenQuadVB, &stride, &offset);
	context->Draw(4, 0);

	// 7. Unbind Resources
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->PSSetShaderResources(0, 1, &nullSRV);

	// 8. Restore previous render state
	context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, oldRTVs, oldDSV);
	context->RSSetViewports(numViewports, oldViewports);

	// Release references
	for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		if (oldRTVs[i]) oldRTVs[i]->Release();
	}
	if (oldDSV) oldDSV->Release();

	// 9. Return the remapped SRV
	return RemappedSRV;
}
