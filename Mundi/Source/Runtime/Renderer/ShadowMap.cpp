#include "pch.h"
#include "ShadowMap.h"

FShadowMap::FShadowMap()
	: Width(0)
	, Height(0)
	, ArraySize(0)
	, ShadowMapTexture(nullptr)
	, ShadowMapSRV(nullptr)
{
	ZeroMemory(&ShadowViewport, sizeof(D3D11_VIEWPORT));
}

FShadowMap::~FShadowMap()
{
	Release();
}

void FShadowMap::Initialize(D3D11RHI* RHI, UINT InWidth, UINT InHeight, UINT InArraySize)
{
	Width = InWidth;
	Height = InHeight;
	ArraySize = InArraySize;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = Width;
	texDesc.Height = Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = ArraySize; // Array of shadow maps
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Typeless를 선언해야만, 텍스쳐 Read & Write 2가지가 가능해집니다.
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // DSV 및 SRV를 사용하도록 설정합니다.
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	HRESULT hr = RHI->GetDevice()->CreateTexture2D(&texDesc, nullptr, &ShadowMapTexture);
	assert(SUCCEEDED(hr), "Failed to create shadow map texture array");

	// Create depth stencil views for each array slice
	ShadowMapDSVs.clear();
	for (UINT i = 0; i < ArraySize; i++)
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

	// Create shader resource view for entire array
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // Read as float in shader
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = ArraySize;

	hr = RHI->GetDevice()->CreateShaderResourceView(ShadowMapTexture, &srvDesc, &ShadowMapSRV);
	assert(SUCCEEDED(hr), "Failed to create shadow map SRV");

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

	UE_LOG("[DEBUG] FShadowMap::BeginRender - ArrayIndex=%d, ArraySize=%d, Width=%d, Height=%d",
		ArrayIndex, ArraySize, Width, Height);

	ID3D11DeviceContext* pContext = RHI->GetDeviceContext();

	// N개의 쉐도우 DSV에서 특정 DSV를 가져옵니다.
	ID3D11DepthStencilView* DSV = ShadowMapDSVs[ArrayIndex];

	UE_LOG("[DEBUG] FShadowMap::BeginRender - DSV = 0x%p", DSV);

	// DSV를 초기화합니다.
	pContext->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Unbind all pixel shader resources (depth-only pass doesn't need textures)
	ID3D11ShaderResourceView* pNullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { nullptr };
	pContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pNullSRVs);

	// Unbind pixel shader (depth-only rendering)
	pContext->PSSetShader(nullptr, nullptr, 0);

	// Set rasterizer state to Shadow (proper culling and depth bias for shadow acne prevention)
	RHI->RSSetState(ERasterizerMode::Shadow);

	// Set depth-stencil state for shadow rendering (depth write enabled, depth test enabled)
	RHI->OMSetDepthStencilState(EComparisonFunc::LessEqual);

	// OM 단계에서, 컬러 렌더링을 하지 않고 오직 깊이 버퍼만 쓰도록 설정합니다.
	ID3D11RenderTargetView* nullRTV = nullptr;
	pContext->OMSetRenderTargets(1, &nullRTV, DSV);

	// Initialize 함수를 통해 갱신된 뷰포트 멤버 변수로 뷰포트를 설정합니다.
	pContext->RSSetViewports(1, &ShadowViewport);
}

void FShadowMap::EndRender(D3D11RHI* RHI)
{
	assert(RHI != nullptr, "RHI is null");

	// Unbind render targets
	ID3D11RenderTargetView* nullRTV = nullptr;
	ID3D11DepthStencilView* nullDSV = nullptr;
	RHI->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullDSV);
}
