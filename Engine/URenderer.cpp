#include "stdafx.h"
#include "URenderer.h"
#include "UClass.h"
int cnt = 0;
/* *
* IASetIndexBuffer https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-iasetindexbuffer
*/

IMPLEMENT_UCLASS(URenderer, UEngineSubsystem)

URenderer::URenderer()
	: device(nullptr)
	, deviceContext(nullptr)
	, swapChain(nullptr)
	, renderTargetView(nullptr)
	, shaderResourceView(nullptr)
	, samplerState(nullptr)
	, depthStencilView(nullptr)
	, constantBuffer(nullptr)
	, fontConstantBuffer(nullptr)
	, rasterizerState(nullptr)
	, hWnd(nullptr)
	, bIsInitialized(false)
{
	ZeroMemory(&viewport, sizeof(viewport));
}

URenderer::~URenderer()
{
	Release();
}

bool URenderer::Initialize(HWND windowHandle)
{
	if (bIsInitialized)
		return true;

	hWnd = windowHandle;

	// Create device and swap chain
	if (!CreateDeviceAndSwapChain(windowHandle))
	{
		LogError("CreateDeviceAndSwapChain", E_FAIL);
		return false;
	}

	// Create render target view
	if (!CreateRenderTargetView())
	{
		LogError("CreateRenderTargetView", E_FAIL);
		return false;
	}

	// Get back buffer size and create depth stencil view
	int32 width, height;
	GetBackBufferSize(width, height);

	if (!CreateDepthStencilView(width, height))
	{
		LogError("CreateDepthStencilView", E_FAIL);
		return false;
	}

	// Setup viewport
	if (!SetupViewport(width, height))
	{
		LogError("SetupViewport", E_FAIL);
		return false;
	}

	if (!CreateRasterizerState())
	{
		LogError("CreateRasterizerState", E_FAIL);
		return false;
	}

	if (!CreateDefaultSampler())
	{
		LogError("SamplerState", E_FAIL);
		return false;
	}

	DirectX::CreateDDSTextureFromFile(device, L"assets/font_transparent.dds", &resource, &shaderResourceView);
	CreateFontConstantBuffer();

	bIsInitialized = true;
	return true;
}

// URenderer.cpp의 CreateShader() 함수를 다음과 같이 수정

bool URenderer::CreateShader()
{
	// Load vertex shader from file
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr;

	// --- 1. 기본(Default) 셰이더 및 레이아웃 생성 ---
	{
		// 1-1. 버텍스 셰이더 컴파일
		// 파일 경로
		// 매크로 정의
		// Include 핸들러
		// 진입점 함수명
		// 셰이더 모델
		// 컴파일 플래그
		// 효과 플래그
		// 컴파일된 셰이더
		// 에러 메시지
		hr = D3DCompileFromFile(L"ShaderW0.vs", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
			{
				OutputDebugStringA("Default Vertex Shader Compile Error:\n");
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				SAFE_RELEASE(errorBlob);
			}
			else { OutputDebugStringA("Failed to find ShaderW0.vs\n"); }
			return false;
		}

		// 버텍스 셰이더 객체 생성 및 TMap에 저장
		ID3D11VertexShader* vs = nullptr;
		hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs);
		if (!CheckResult(hr, "CreateVertexShader"))
		{
			SAFE_RELEASE(vsBlob);
			return false;
		}
		VertexShaders["Default"] = vs;

		// 입력 레이아웃 생성 및 TMap에 저장
		D3D11_INPUT_ELEMENT_DESC defaultLayoutDesc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		ID3D11InputLayout* layout = nullptr;
		hr = device->CreateInputLayout(defaultLayoutDesc, ARRAYSIZE(defaultLayoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout);
		if (!CheckResult(hr, "CreateInputLayout (Default)"))
		{
			SAFE_RELEASE(vsBlob);
			return false;
		}
		InputLayouts["Default"] = layout;
		SAFE_RELEASE(vsBlob); // VS와 Layout 생성을 마쳤으므로 Blob 해제

		// 정점 색상용 픽셀 셰이더 컴파일 및 TMap에 저장
		hr = D3DCompileFromFile(L"ShaderW0.ps", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob) {
				OutputDebugStringA("Default Pixel Shader Compile Error:\n");
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				SAFE_RELEASE(errorBlob);
			}
			else { OutputDebugStringA("Failed to find ShaderW0.ps\n"); }
			return false;
		}
		ID3D11PixelShader* ps = nullptr;
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps);
		if (!CheckResult(hr, "CreatePixelShader"))
		{
			SAFE_RELEASE(psBlob);
			return false;
		}
		PixelShaders["Default"] = ps;
		SAFE_RELEASE(psBlob);
	}
	// 2. 폰트(Font) 셰이더 및 레이아웃 생성
	{
		// Font 버텍스 셰이더 컴파일 및 TMap에 저장
		hr = D3DCompileFromFile(L"ShaderFont.hlsl", nullptr, nullptr, "VS_Main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
			{
				OutputDebugStringA("Font Vertex Shader Compile Error:\n");
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				SAFE_RELEASE(errorBlob);
			}
			else { OutputDebugStringA("Failed to find ShaderFont.vs\n"); }
			return false;
		}

		ID3D11VertexShader* vs = nullptr;
		hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs);
		if (!CheckResult(hr, "CreateVertexShader (Font)"))
		{
			SAFE_RELEASE(vsBlob);
			return false;
		}
		VertexShaders["Font"] = vs;

		// Font 입력 레이아웃 생성 및 TMap에 저장
		D3D11_INPUT_ELEMENT_DESC fontLayoutDesc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		ID3D11InputLayout* layout = nullptr;
		hr = device->CreateInputLayout(fontLayoutDesc, ARRAYSIZE(fontLayoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout);
		if (!CheckResult(hr, "CreateInputLayout (Font)"))
		{
			SAFE_RELEASE(vsBlob);
			return false;
		}
		InputLayouts["Font"] = layout;
		SAFE_RELEASE(vsBlob); // Blob 해제

		// 폰트 픽셀 셰이더 컴파일 및 TMap에 저장
		hr = D3DCompileFromFile(L"ShaderFont.hlsl", nullptr, nullptr, "PS_Main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob) {
				OutputDebugStringA("Font Pixel Shader Compile Error:\n");
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				SAFE_RELEASE(errorBlob);
			}
			else { OutputDebugStringA("Failed to find ShaderFont.ps\n"); }
			return false;
		}
		ID3D11PixelShader* ps = nullptr;
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps);
		if (!CheckResult(hr, "CreatePixelShader (Font)"))
		{
			SAFE_RELEASE(psBlob);
			return false;
		}
		PixelShaders["Font"] = ps;
		SAFE_RELEASE(psBlob);
	}
	// 3. 라인(Line) 셰이더 및 레이아웃 생성 ---
	{
		// 라인 버텍스 셰이더 컴파일 및 TMap에 저장
		hr = D3DCompileFromFile(L"ShaderLine.hlsl", nullptr, nullptr, "VS_Main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
			{
				OutputDebugStringA("Line Vertex Shader Compile Error:\n");
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				SAFE_RELEASE(errorBlob);
			}
			else { OutputDebugStringA("Failed to find ShaderLine.vs\n"); }
			return false;
		}

		// 라인 버텍스 셰이더 객체 생성 및 TMap에 저장
		ID3D11VertexShader* vs = nullptr;
		hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs);
		if (!CheckResult(hr, "CreateVertexShader (Line)"))
		{
			SAFE_RELEASE(vsBlob);
			return false;
		}
		VertexShaders["Line"] = vs;

		// 라인 입력 레이아웃 생성 및 TMap에 저장
		D3D11_INPUT_ELEMENT_DESC lineLayoutDesc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		ID3D11InputLayout* layout = nullptr;
		hr = device->CreateInputLayout(lineLayoutDesc, ARRAYSIZE(lineLayoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout);
		if (!CheckResult(hr, "CreateInputLayout (Line)"))
		{
			SAFE_RELEASE(vsBlob);
			return false;
		}
		InputLayouts["Line"] = layout;
		SAFE_RELEASE(vsBlob); // Blob 해제

		// 라인 픽셀 셰이더 컴파일 및 TMap에 저장
		hr = D3DCompileFromFile(L"ShaderLine.hlsl", nullptr, nullptr, "PS_Main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob) {
				OutputDebugStringA("Line Pixel Shader Compile Error:\n");
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				SAFE_RELEASE(errorBlob);
			}
			else { OutputDebugStringA("Failed to find ShaderLine.ps\n"); }
			return false;
		}
		ID3D11PixelShader* ps = nullptr;
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps);
		if (!CheckResult(hr, "CreatePixelShader (Line)"))
		{
			SAFE_RELEASE(psBlob);
			return false;
		}
		PixelShaders["Line"] = ps;
		SAFE_RELEASE(psBlob);
		batchLineList.vertexShader = VertexShaders["Line"];
		batchLineList.pixelShader = PixelShaders["Line"];
		batchLineList.inputLayout = InputLayouts["Line"];
	}
	return true;
}

bool URenderer::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;           // 뒷면 제거
	rasterizerDesc.FrontCounterClockwise = TRUE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;

	HRESULT hr = device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
	return CheckResult(hr, "CreateRasterizerState");
}

bool URenderer::CreateDefaultSampler()
{
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT hr = device->CreateSamplerState(&samplerDesc, &samplerState);
	return CheckResult(hr, "CreateDefaultSampler");
}

bool URenderer::CreateConstantBuffer()
{
	// 월드 좌표용 상수 버퍼
	{
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = sizeof(CBTransform);   // ← 변경
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		return CheckResult(device->CreateBuffer(&bufferDesc, nullptr, &constantBuffer), "CreateConstantBuffer");
	}
}

bool URenderer::CreateFontConstantBuffer()
{
	// 폰트 uv용 상수 버퍼
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(float) * 4; // UVRect 4 floats (16 bytes)
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	return CheckResult(device->CreateBuffer(&bufferDesc, nullptr, &fontConstantBuffer), "fontConstantBuffer");
}

ID3D11Buffer* URenderer::CreateVertexBuffer(const void* data, size_t sizeInBytes)
{
	if (!device || !data || sizeInBytes == 0)
		return nullptr;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = static_cast<UINT>(sizeInBytes);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = data;

	ID3D11Buffer* buffer = nullptr;
	HRESULT hr = device->CreateBuffer(&bufferDesc, &initData, &buffer);

	if (FAILED(hr))
	{
		LogError("CreateVertexBuffer", hr);
		return nullptr;
	}

	return buffer;
}

bool URenderer::UpdateConstantBuffer(const void* data, size_t sizeInBytes)
{
	if (!constantBuffer || !data)
		return false;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	if (FAILED(hr))
	{
		LogError("Map ConstantBuffer", hr);
		return false;
	}

	memcpy(mappedResource.pData, data, sizeInBytes);
	deviceContext->Unmap(constantBuffer, 0);

	return true;
}

void URenderer::UpdateFontConstantBuffer(const FConstantFont& r)
{
	const float uv[4] = { r.u0, r.v0, r.u1, r.v1 };
	deviceContext->UpdateSubresource(fontConstantBuffer, 0, nullptr, uv, 0, 0);
	deviceContext->VSSetConstantBuffers(2, 1, &fontConstantBuffer);
	deviceContext->PSSetConstantBuffers(2, 1, &fontConstantBuffer);
}


void URenderer::BeginBatchLineList()
{
	batchLineList.Clear();
}

void URenderer::SubmitLineList(const UMesh* mesh)
{
	if (mesh == nullptr || mesh->PrimitiveType != D3D11_PRIMITIVE_TOPOLOGY_LINELIST) { return; }

	SubmitLineList(mesh->Vertices, mesh->Indices);
}

void URenderer::SubmitLineList(const TArray<FVertexPosColor4>& vertices, const TArray<uint32>& indices)
{
	if (vertices.empty()) { return; }

	const size_t base = batchLineList.Vertices.size();

	batchLineList.Vertices.insert(batchLineList.Vertices.end(), vertices.begin(), vertices.end());

	if (!indices.empty())
	{
		batchLineList.Indices.reserve(batchLineList.Indices.size() + indices.size());
		for (uint32 idx : indices)
		{
			batchLineList.Indices.push_back(static_cast<uint32>(base + idx));
		}
	}
	else
	{
		// 인덱스 없음 → (0,1),(2,3)... 페어링해서 생성
		const size_t n = vertices.size();
		const bool hasOdd = (n & 1) != 0;
		const size_t pairs = n >> 1; // n/2

		if (hasOdd)
		{
			// 마지막 남는 1개는 버림 (라인 페어 불가). 필요하면 여기서 디버그 로그만 찍자.
			OutputDebugStringA("[Batch] LineList mesh has odd vertex count; dropping last vertex.\n");
		}

		batchLineList.Indices.reserve(batchLineList.Indices.size() + pairs * 2);
		for (size_t p = 0; p < pairs; ++p)
		{
			batchLineList.Indices.push_back(static_cast<uint32>(base + (p * 2 + 0)));
			batchLineList.Indices.push_back(static_cast<uint32>(base + (p * 2 + 1)));
		}
	}
}

void URenderer::FlushBatchLineList()
{
	const size_t vCount = batchLineList.Vertices.size();
	const size_t iCount = batchLineList.Indices.size();

	if (vCount == 0 && iCount == 0)
		return;

	// 단위행렬(또는 VP만)로 업로드: 라인은 이미 월드로 변환해둠
	SetModel(FMatrix::IdentityMatrix(), FVector4(1, 1, 1, 1), false);
	// 1) GPU 버퍼 확보/확장
	EnsureBatchCapacity(batchLineList, vCount, iCount);

	// 2) Map & copy
	D3D11_MAPPED_SUBRESOURCE map{};
	HRESULT hr = deviceContext->Map(batchLineList.VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (SUCCEEDED(hr))
	{
		memcpy(map.pData, batchLineList.Vertices.data(), vCount * sizeof(FVertexPosColor4));
		deviceContext->Unmap(batchLineList.VertexBuffer, 0);
	}

	hr = deviceContext->Map(batchLineList.IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (SUCCEEDED(hr))
	{
		memcpy(map.pData, batchLineList.Indices.data(), iCount * sizeof(uint32));
		deviceContext->Unmap(batchLineList.IndexBuffer, 0);
	}

	// 4) IA 바인딩 & 드로우 (1콜)
	UINT stride = sizeof(FVertexPosColor4), offset = 0;
	ID3D11Buffer* vb = batchLineList.VertexBuffer;
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	deviceContext->IASetIndexBuffer(batchLineList.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	deviceContext->IASetInputLayout(batchLineList.inputLayout);
	deviceContext->VSSetShader(batchLineList.vertexShader, nullptr, 0);
	deviceContext->PSSetShader(batchLineList.pixelShader, nullptr, 0);

	deviceContext->DrawIndexed(static_cast<UINT>(iCount), 0, 0);
	UE_LOG("[Batch] LineList DrawIndexed iCount=%u (draw #%d)\n", (UINT)iCount, cnt++);
}

void URenderer::LogError(const char* function, HRESULT hr)
{
	char errorMsg[512];
	sprintf_s(errorMsg, "URenderer::%s failed with HRESULT: 0x%08X", function, hr);
	OutputDebugStringA(errorMsg);
}

bool URenderer::CheckResult(HRESULT hr, const char* function)
{
	if (FAILED(hr))
	{
		LogError(function, hr);
		return false;
	}
	return true;
}

// URenderer.cpp의 나머지 함수들 (기존 코드에 추가)

void URenderer::Release()
{
	if (!bIsInitialized)
		return;

	ReleaseShader();
	ReleaseConstantBuffer();

	SAFE_RELEASE(rasterizerState);
	SAFE_RELEASE(depthStencilView);
	SAFE_RELEASE(renderTargetView);
	SAFE_RELEASE(shaderResourceView);
	SAFE_RELEASE(samplerState);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(deviceContext);
	SAFE_RELEASE(device);
	SAFE_RELEASE(resource);

	bIsInitialized = false;
	hWnd = nullptr;
}

void URenderer::ReleaseShader()
{
	for (auto& pair : PixelShaders)
	{
		SAFE_RELEASE(pair.second);
	}
	PixelShaders.clear();

	for (auto& pair : VertexShaders)
	{
		SAFE_RELEASE(pair.second);
	}
	VertexShaders.clear();

	for (auto& pair : InputLayouts)
	{
		SAFE_RELEASE(pair.second);
	}
	InputLayouts.clear();
}

void URenderer::ReleaseConstantBuffer()
{
	SAFE_RELEASE(constantBuffer);
	SAFE_RELEASE(fontConstantBuffer);
}


bool URenderer::ReleaseVertexBuffer(ID3D11Buffer* buffer)
{
	if (buffer)
	{
		buffer->Release();
		return true;
	}
	return false;
}

bool URenderer::ReleaseIndexBuffer(ID3D11Buffer* buffer)
{
	if (buffer)
	{
		buffer->Release();
		return true;
	}
	return false;
}

ID3D11Buffer* URenderer::CreateIndexBuffer(const void* data, size_t sizeInBytes)
{
	if (!device || !data || sizeInBytes == 0)
		return nullptr;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = static_cast<UINT>(sizeInBytes);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = data;

	ID3D11Buffer* buffer = nullptr;
	HRESULT hr = device->CreateBuffer(&bufferDesc, &initData, &buffer);

	if (FAILED(hr))
	{
		LogError("CreateIndexBuffer", hr);
		return nullptr;
	}

	return buffer;
}

ID3D11Texture2D* URenderer::CreateTexture2D(int32 width, int32 height, DXGI_FORMAT format, const void* data)
{
	if (!device || width <= 0 || height <= 0)
		return nullptr;

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = static_cast<UINT>(width);
	textureDesc.Height = static_cast<UINT>(height);
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
	D3D11_SUBRESOURCE_DATA initData = {};
	if (data)
	{
		initData.pSysMem = data;
		initData.SysMemPitch = width * 4; // Assuming 4 bytes per pixel (RGBA)
		pInitData = &initData;
	}

	ID3D11Texture2D* texture = nullptr;
	HRESULT hr = device->CreateTexture2D(&textureDesc, pInitData, &texture);

	if (FAILED(hr))
	{
		LogError("CreateTexture2D", hr);
		return nullptr;
	}

	return texture;
}

ID3D11ShaderResourceView* URenderer::CreateShaderResourceView(ID3D11Texture2D* texture)
{
	if (!device || !texture)
		return nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Assuming RGBA format
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* srv = nullptr;
	HRESULT hr = device->CreateShaderResourceView(texture, &srvDesc, &srv);

	if (FAILED(hr))
	{
		LogError("CreateShaderResourceView", hr);
		return nullptr;
	}

	return srv;
}

bool URenderer::ReleaseTexture(ID3D11Texture2D* texture)
{
	if (texture)
	{
		texture->Release();
		return true;
	}
	return false;
}

bool URenderer::ReleaseShaderResourceView(ID3D11ShaderResourceView* srv)
{
	if (srv)
	{
		srv->Release();
		return true;
	}
	return false;
}

void URenderer::Prepare()
{
	if (!deviceContext)
		return;

	// Set render target and depth stencil view
	deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

	// Set viewport
	deviceContext->RSSetViewports(1, &currentViewport);

	// Clear render target and depth stencil
	Clear();
}

void URenderer::PrepareShader()
{
	if (!deviceContext)
		return;

	// Set primitive topology (default to triangle list)
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set rasterizer state (와인딩 순서 적용)
	if (rasterizerState)
	{
		deviceContext->RSSetState(rasterizerState);
	}

	// Set constant buffer
	if (constantBuffer)
	{
		deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
	}

}

void URenderer::SwapBuffer()
{
	if (swapChain)
	{
		swapChain->Present(1, 0); // VSync enabled
	}
}

void URenderer::Clear(float r, float g, float b, float a)
{
	if (!deviceContext)
		return;

	float clearColor[4] = { r, g, b, a };

	if (renderTargetView)
	{
		deviceContext->ClearRenderTargetView(renderTargetView, clearColor);
	}

	if (depthStencilView)
	{
		deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
}

void URenderer::DrawIndexed(UINT indexCount, UINT startIndexLocation, INT baseVertexLocation)
{
	if (deviceContext)
	{
		deviceContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
	}
}

void URenderer::Draw(UINT vertexCount, UINT startVertexLocation)
{
	if (deviceContext)
	{
		deviceContext->Draw(vertexCount, startVertexLocation);
	}
}

void URenderer::DrawMesh(UMesh* mesh)
{
	if (!mesh || !mesh->IsInitialized())
		return;
	
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &mesh->VertexBuffer, &mesh->Stride, &offset);
	deviceContext->IASetPrimitiveTopology(mesh->PrimitiveType);

	if (mesh->Type == EVertexType::VERTEX_POS_UV)
	{
		deviceContext->IASetInputLayout(GetInputLayout("Font"));
		deviceContext->VSSetShader(GetVertexShader("Font"), nullptr, 0);
		deviceContext->PSSetShader(GetPixelShader("Font"), nullptr, 0);

		if (shaderResourceView) { deviceContext->PSSetShaderResources(0, 1, &shaderResourceView); }
		if (samplerState) { deviceContext->PSSetSamplers(0, 1, &samplerState); }

		// 3) Draw
		if (mesh->IndexBuffer && mesh->NumIndices > 0)
		{
			deviceContext->IASetIndexBuffer(mesh->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			deviceContext->DrawIndexed(mesh->NumIndices, 0, 0);
		}
		else
		{
			deviceContext->Draw(mesh->NumVertices, 0);
		}

	}
	else if (mesh->Type == EVertexType::VERTEX_POS_COLOR)
	{
		deviceContext->IASetInputLayout(GetInputLayout("Default"));
		deviceContext->VSSetShader(GetVertexShader("Default"), nullptr, 0);
		deviceContext->PSSetShader(GetPixelShader("Default"), nullptr, 0);

		// 인덱스 버퍼도 가지고 있으면 아래 방식으로 Draw
		if (mesh->NumIndices > 0)
		{
			deviceContext->IASetIndexBuffer(mesh->IndexBuffer, DXGI_FORMAT_R32_UINT, offset);
			deviceContext->DrawIndexed(mesh->NumIndices, 0, 0);
		}
		// 정점 버퍼만 가지고 있으면 아래 방식으로 Draw
		else if (mesh->NumIndices == 0)
		{
			deviceContext->Draw(mesh->NumVertices, 0);
			UE_LOG("Draw Call %d \n", cnt++);
		}
	}

}

void URenderer::DrawMeshOnTop(UMesh* mesh)
{
	if (!mesh || !mesh->IsInitialized())
		return;

	// 1) depth test off 상태 준비
	// Create a depth-stencil state with depth testing disabled
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = FALSE;  // disable depth testing
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

	ID3D11DepthStencilState* pDSState = nullptr;
	HRESULT hr = device->CreateDepthStencilState(&dsDesc, &pDSState);
	if (FAILED(hr))
	{
		LogError("CreateDepthStencilState (DrawMeshOnTop)", hr);
		return;
	}

	// 백업
	// Backup current depth-stencil state
	ID3D11DepthStencilState* pOldState = nullptr;
	UINT stencilRef = 0;
	deviceContext->OMGetDepthStencilState(&pOldState, &stencilRef);

	// 적용
	// Set new state (no depth test)
	deviceContext->OMSetDepthStencilState(pDSState, 0);
	deviceContext->IASetInputLayout(GetInputLayout("Default"));
	deviceContext->VSSetShader(GetVertexShader("Default"), nullptr, 0);
	deviceContext->PSSetShader(GetPixelShader("Default"), nullptr, 0);

	// 2) 공통 셋업
	// Draw mesh
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &mesh->VertexBuffer, &mesh->Stride, &offset);
	deviceContext->IASetPrimitiveTopology(mesh->PrimitiveType);

	// 3) 인덱스 있으면 DrawIndexed, 없으면 Draw
	if (mesh->IndexBuffer && mesh->NumIndices > 0)
	{
		deviceContext->IASetIndexBuffer(mesh->IndexBuffer, DXGI_FORMAT_R32_UINT, /*OffsetBytes=*/0);
		deviceContext->DrawIndexed(mesh->NumIndices, 0, 0);
	}
	else
	{
		deviceContext->Draw(mesh->NumVertices, 0);
	}

	// 4) 복원 & 해제
	// Restore previous depth state
	deviceContext->OMSetDepthStencilState(pOldState, stencilRef);

	// Release local COM objects
	SAFE_RELEASE(pOldState);
	SAFE_RELEASE(pDSState);
}

void URenderer::SetVertexBuffer(ID3D11Buffer* buffer, UINT stride, UINT offset)
{
	if (deviceContext && buffer)
	{
		deviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
	}
}

void URenderer::SetIndexBuffer(ID3D11Buffer* buffer, DXGI_FORMAT format)
{
	if (deviceContext && buffer)
	{
		deviceContext->IASetIndexBuffer(buffer, format, 0);
	}
}

void URenderer::SetConstantBuffer(ID3D11Buffer* buffer, UINT slot)
{
	if (deviceContext && buffer)
	{
		deviceContext->VSSetConstantBuffers(slot, 1, &buffer);
	}
}

void URenderer::SetTexture(ID3D11ShaderResourceView* srv, UINT slot)
{
	if (deviceContext && srv)
	{
		deviceContext->PSSetShaderResources(slot, 1, &srv);
	}
}

bool URenderer::ResizeBuffers(int32 width, int32 height)
{
	if (!swapChain || width <= 0 || height <= 0)
		return false;

	// Release render target view before resizing
	SAFE_RELEASE(renderTargetView);
	SAFE_RELEASE(depthStencilView);

	// Resize swap chain buffers
	HRESULT hr = swapChain->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height),
		DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr))
	{
		LogError("ResizeBuffers", hr);
		return false;
	}

	// Recreate render target view
	if (!CreateRenderTargetView())
	{
		return false;
	}

	// Recreate depth stencil view
	if (!CreateDepthStencilView(width, height))
	{
		return false;
	}

	// Update viewport
	return SetupViewport(width, height);
}

bool URenderer::CheckDeviceState()
{
	if (!device)
		return false;

	HRESULT hr = device->GetDeviceRemovedReason();
	if (FAILED(hr))
	{
		LogError("Device Lost", hr);
		return false;
	}

	return true;
}

void URenderer::GetBackBufferSize(int32& width, int32& height)
{
	width = height = 0;

	if (!swapChain)
		return;

	ID3D11Texture2D* backBuffer = nullptr;
	HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);

	if (SUCCEEDED(hr) && backBuffer)
	{
		D3D11_TEXTURE2D_DESC desc;
		backBuffer->GetDesc(&desc);
		width = static_cast<int32>(desc.Width);
		height = static_cast<int32>(desc.Height);
		backBuffer->Release();
	}
}

// Private helper functions

bool URenderer::CreateDeviceAndSwapChain(HWND windowHandle)
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = 0; // Use window size
	swapChainDesc.BufferDesc.Height = 0;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = windowHandle;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,                    // Use default adapter
		D3D_DRIVER_TYPE_HARDWARE,   // Hardware acceleration
		nullptr,                    // No software module
		0,                          // No flags
		nullptr,                    // Use default feature levels
		0,                          // Number of feature levels
		D3D11_SDK_VERSION,          // SDK version
		&swapChainDesc,             // Swap chain description
		&swapChain,                 // Swap chain output
		&device,                    // Device output
		&featureLevel,              // Feature level output
		&deviceContext              // Device context output
	);

	return CheckResult(hr, "D3D11CreateDeviceAndSwapChain");
}

bool URenderer::CreateRenderTargetView()
{
	ID3D11Texture2D* backBuffer = nullptr;
	HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);

	if (FAILED(hr))
	{
		LogError("GetBuffer", hr);
		return false;
	}

	hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
	backBuffer->Release();

	return CheckResult(hr, "CreateRenderTargetView");
}

bool URenderer::CreateDepthStencilView(int32 width, int32 height)
{
	D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	depthStencilDesc.Width = static_cast<UINT>(width);
	depthStencilDesc.Height = static_cast<UINT>(height);
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D* depthStencilBuffer = nullptr;
	HRESULT hr = device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer);

	if (FAILED(hr))
	{
		LogError("CreateTexture2D (DepthStencil)", hr);
		return false;
	}

	hr = device->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView);
	depthStencilBuffer->Release();

	return CheckResult(hr, "CreateDepthStencilView");
}

bool URenderer::SetupViewport(int32 width, int32 height)
{
	viewport.Width = static_cast<FLOAT>(width);
	viewport.Height = static_cast<FLOAT>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	// 기본은 풀 윈도우
	currentViewport = viewport;
	return true;
}

void URenderer::EnsureBatchCapacity(FBatchLineList& B, size_t vNeed, size_t iNeed)
{
	if (vNeed > B.MaxVertex)
	{
		if (B.VertexBuffer)
		{
			B.VertexBuffer->Release();
		}

		B.MaxVertex = max(vNeed, B.MaxVertex * 2 + size_t(1024));

		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.ByteWidth = static_cast<UINT>(B.MaxVertex * sizeof(FVertexPosColor4));
		HRESULT hr = device->CreateBuffer(&bd, nullptr, &B.VertexBuffer);
		CheckResult(hr, "EnsureBatchCapacity.Create VB");
	}

	// Index buffer
	if (iNeed > B.MaxIndex)
	{
		if (B.IndexBuffer)
		{
			B.IndexBuffer->Release();
		}

		B.MaxIndex = max(iNeed, B.MaxIndex * 2 + size_t(2048));

		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.ByteWidth = static_cast<UINT>(B.MaxIndex * sizeof(uint32));
		HRESULT hr = device->CreateBuffer(&bd, nullptr, &B.IndexBuffer);
		CheckResult(hr, "EnsureBatchCapacity.Create IB");
	}
}

void URenderer::SetViewProj(const FMatrix& V, const FMatrix& P)
{
	// row-vector 규약이면 곱셈 순서는 V*P가 아니라, 최종적으로 v*M*V*P가 되도록
	// 프레임 캐시엔 VP = V * P 저장
	mVP = V * P;
	// 여기서는 상수버퍼 업로드 안 함 (오브젝트에서 M과 합쳐서 업로드)
}

void URenderer::SetModel(const FMatrix& M, const FVector4& color, bool bIsSelected)
{
	// per-object: MVP = M * VP
	FMatrix MVP = M * mVP;
	CopyRowMajor(mCBData.MVP, MVP);
	memcpy(mCBData.MeshColor, &color, sizeof(float) * 4);
	mCBData.IsSelected = bIsSelected ? 1.0f : 0.0f;
	UpdateConstantBuffer(&mCBData, sizeof(mCBData));
}

D3D11_VIEWPORT URenderer::MakeAspectFitViewport(int32 winW, int32 winH) const
{
	D3D11_VIEWPORT vp{};
	vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;

	float wa = (winH > 0) ? (float)winW / (float)winH : targetAspect;
	if (wa > targetAspect)
	{
		vp.Height = (float)winH;
		vp.Width = vp.Height * targetAspect;
		vp.TopLeftY = 0.0f;
		vp.TopLeftX = 0.5f * (winW - vp.Width);
	}
	else
	{
		vp.Width = (float)winW;
		vp.Height = vp.Width / targetAspect;
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.5f * (winH - vp.Height);
	}
	return vp;
}
// URenderer.cpp
void URenderer::SubmitLineList(const TArray<FVertexPosColor4>& vertices,
	const TArray<uint32>& indices,
	const FMatrix& model)
{
	if (vertices.empty()) return;

	// 1) 월드로 미리 변환해서 버퍼에 누적
	const size_t base = batchLineList.Vertices.size();
	batchLineList.Vertices.reserve(base + vertices.size());

	for (auto v : vertices) {
		// 위치만 변환(색은 그대로)
		TransformPosRow(v.x, v.y, v.z, model);
		batchLineList.Vertices.push_back(v);
	}

	// 2) 인덱스 오프셋 적용해서 누적
	if (!indices.empty()) {
		batchLineList.Indices.reserve(batchLineList.Indices.size() + indices.size());
		for (uint32 idx : indices) batchLineList.Indices.push_back((uint32)(base + idx));
	}
	else {
		// (0,1),(2,3)...
		const size_t n = vertices.size();
		const size_t pairs = n >> 1;
		batchLineList.Indices.reserve(batchLineList.Indices.size() + pairs * 2);
		for (size_t p = 0; p < pairs; ++p) {
			batchLineList.Indices.push_back((uint32)(base + p * 2 + 0));
			batchLineList.Indices.push_back((uint32)(base + p * 2 + 1));
		}
	}
}
