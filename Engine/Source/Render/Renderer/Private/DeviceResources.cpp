#include "pch.h"
#include "Render/Renderer/Public/DeviceResources.h"

UDeviceResources::UDeviceResources(HWND InWindowHandle)
	: PreviousFrameDepthSRV(nullptr)
{
	Create(InWindowHandle);
}

UDeviceResources::~UDeviceResources()
{
	Release();
}

void UDeviceResources::Create(HWND InWindowHandle)
{
	RECT ClientRect;
	GetClientRect(InWindowHandle, &ClientRect);
	Width = ClientRect.right - ClientRect.left;
	Height = ClientRect.bottom - ClientRect.top;

	CreateDeviceAndSwapChain(InWindowHandle);
	CreateFrameBuffer();
	CreateDepthBuffer();
	CreateSceneColorTargets();
	CreateFactories();
}

void UDeviceResources::Release()
{
	ReleaseFactories();
	ReleaseFrameBuffer();
	ReleaseDepthBuffer();
	ReleaseSceneColorTargets();
	ReleaseDeviceAndSwapChain();
}

/**
 * @brief Direct3D 장치 및 스왑 체인을 생성하는 함수
 * @param InWindowHandle
 */
void UDeviceResources::CreateDeviceAndSwapChain(HWND InWindowHandle)
{
	// 지원하는 Direct3D 기능 레벨을 정의
	D3D_FEATURE_LEVEL featurelevels[] = {D3D_FEATURE_LEVEL_11_0};

	// 스왑 체인 설정 구조체 초기화
	DXGI_SWAP_CHAIN_DESC SwapChainDescription = {};
	SwapChainDescription.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
	SwapChainDescription.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
	SwapChainDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
	SwapChainDescription.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
	SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
	SwapChainDescription.BufferCount = 2; // 더블 버퍼링
	SwapChainDescription.OutputWindow = InWindowHandle; // 렌더링할 창 핸들
	SwapChainDescription.Windowed = TRUE; // 창 모드
	SwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

	// Direct3D 장치와 스왑 체인을 생성
	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
	                                           D3D11_CREATE_DEVICE_BGRA_SUPPORT,
	                                           //D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
	                                           featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
	                                           &SwapChainDescription, &SwapChain, &Device, nullptr, &DeviceContext);

	if (FAILED(hr))
	{
		assert(!"Failed To Create SwapChain");
	}

	// 생성된 스왑 체인의 정보 가져오기
	SwapChain->GetDesc(&SwapChainDescription);

	// Viewport Info 업데이트
	ViewportInfo = {
		0.0f, 0.0f, static_cast<float>(SwapChainDescription.BufferDesc.Width),
		static_cast<float>(SwapChainDescription.BufferDesc.Height), 0.0f, 1.0f
	};
}

/**
 * @brief Direct3D 장치 및 스왑 체인을 해제하는 함수
 */
void UDeviceResources::ReleaseDeviceAndSwapChain()
{
	if (DeviceContext)
	{
		// 남아있는 GPU 명령 실행
		DeviceContext->Flush();
	}

	if (SwapChain)
	{
		SwapChain->Release();
		SwapChain = nullptr;
	}

	// DX 메모리 Leak 디버깅용 함수
	ID3D11Debug* DebugPointer = nullptr;
	HRESULT Result = GetDevice()->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&DebugPointer));
	if (SUCCEEDED(Result))
	{
		DebugPointer->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		DebugPointer->Release();
	}

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}

	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}
}

/**
 * @brief FrameBuffer 생성 함수
 */
void UDeviceResources::CreateFrameBuffer()
{
	// 스왑 체인으로부터 백 버퍼 텍스처 가져오기
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

	// 렌더 타겟 뷰 생성
	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
	framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

	Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
}

/**
 * @brief 프레임 버퍼를 해제하는 함수
 */
void UDeviceResources::ReleaseFrameBuffer()
{
	if (FrameBuffer)
	{
		FrameBuffer->Release();
		FrameBuffer = nullptr;
	}

	if (FrameBufferRTV)
	{
		FrameBufferRTV->Release();
		FrameBufferRTV = nullptr;
	}
}

void UDeviceResources::CreateDepthBuffer()
{
	D3D11_TEXTURE2D_DESC dsDesc = {};
	dsDesc.Width = Width;
	dsDesc.Height = Height;
	dsDesc.MipLevels = 1;
	dsDesc.ArraySize = 1;
	dsDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Changed to R32_TYPELESS for 32-bit float depth
	dsDesc.SampleDesc.Count = 1;
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.Usage = D3D11_USAGE_DEFAULT;
	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	dsDesc.CPUAccessFlags = 0;
	dsDesc.MiscFlags = 0;
	Device->CreateTexture2D(&dsDesc, nullptr, &DepthBuffer);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // Changed DSV format to D32_FLOAT
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	Device->CreateDepthStencilView(DepthBuffer, &dsvDesc, &DepthStencilView);

	D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
	ShaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT; // Changed SRV format to R32_FLOAT
	ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ShaderResourceViewDesc.Texture2D.MipLevels = 1;
	Device->CreateShaderResourceView(DepthBuffer, &ShaderResourceViewDesc, &DepthShaderResourceView);

	D3D11_SAMPLER_DESC sd = {};
	sd.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sd.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.MipLODBias     = 0.0f;
	sd.MaxAnisotropy  = 1;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER; // 일반 SRV 읽기
	sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 0.0f;
	sd.MinLOD         = 0.0f;
	sd.MaxLOD         = D3D11_FLOAT32_MAX;

	Device->CreateSamplerState(&sd, &DepthSamplerState);
}

void UDeviceResources::ReleaseDepthBuffer()
{
	if (DepthStencilView)
	{
		DepthStencilView->Release();
		DepthStencilView = nullptr;
	}
	if (DepthBuffer)
	{
		DepthBuffer->Release();
		DepthBuffer = nullptr;
	}
	if (DepthSamplerState)
	{
		DepthSamplerState->Release();
		DepthSamplerState = nullptr;
	}
	if (DepthShaderResourceView)
	{
		DepthShaderResourceView->Release();
		DepthShaderResourceView = nullptr;
	}
	if (PreviousFrameDepthSRV)
	{
		PreviousFrameDepthSRV->Release();
		PreviousFrameDepthSRV = nullptr;
	}
}

void UDeviceResources::CreateSceneColorTargets()
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    SwapChain->GetDesc(&scd);
    UINT w = scd.BufferDesc.Width  ? scd.BufferDesc.Width  : Width;
    UINT h = scd.BufferDesc.Height ? scd.BufferDesc.Height : Height;

    // 혹시 남아있으면 먼저 정리
    ReleaseSceneColorTargets();

    // HDR/Linear 오프스크린 컬러 텍스처
    D3D11_TEXTURE2D_DESC td = {};
    td.Width              = std::max(1u, w);
    td.Height             = std::max(1u, h);
    td.MipLevels          = 1;
    td.ArraySize          = 1;
    td.Format             = DXGI_FORMAT_R16G16B16A16_FLOAT;        // HDR/Linear
    td.SampleDesc.Count   = 1;                                     // 포스트용은 non-MSAA 권장
    td.SampleDesc.Quality = 0;
    td.Usage              = D3D11_USAGE_DEFAULT;
    td.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags     = 0;
    td.MiscFlags          = 0;

    HRESULT hr = Device->CreateTexture2D(&td, nullptr, &SceneColorTex);
    if (FAILED(hr)) { assert(!"CreateTexture2D(SceneColorTex) failed"); return; }

    // RTV
    D3D11_RENDER_TARGET_VIEW_DESC rtv = {};
    rtv.Format        = td.Format;
    rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtv.Texture2D.MipSlice = 0;
    hr = Device->CreateRenderTargetView(SceneColorTex, &rtv, &SceneColorRTV);
    if (FAILED(hr)) { assert(!"CreateRenderTargetView(SceneColorRTV) failed"); return; }

    // SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Format                    = td.Format;
    srv.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MostDetailedMip = 0;
    srv.Texture2D.MipLevels       = 1;
    hr = Device->CreateShaderResourceView(SceneColorTex, &srv, &SceneColorSRV);
    if (FAILED(hr)) { assert(!"CreateShaderResourceView(SceneColorSRV) failed"); return; }
}

void UDeviceResources::ReleaseSceneColorTargets()
{
    if (SceneColorSRV) { SceneColorSRV->Release(); SceneColorSRV = nullptr; }
    if (SceneColorRTV) { SceneColorRTV->Release(); SceneColorRTV = nullptr; }
    if (SceneColorTex) { SceneColorTex->Release(); SceneColorTex = nullptr; }
}

void UDeviceResources::UpdateViewport(float InMenuBarHeight)
{
	DXGI_SWAP_CHAIN_DESC SwapChainDescription = {};
	SwapChain->GetDesc(&SwapChainDescription);

	// 전체 화면 크기
	float FullWidth = static_cast<float>(SwapChainDescription.BufferDesc.Width);
	float FullHeight = static_cast<float>(SwapChainDescription.BufferDesc.Height);

	// 메뉴바 아래에 위치하도록 뷰포트 조정
	ViewportInfo = {
		0.0f,
		InMenuBarHeight,
		FullWidth,
		FullHeight - InMenuBarHeight,
		0.0f,
		1.0f
	};

	Width = SwapChainDescription.BufferDesc.Width;
	Height = SwapChainDescription.BufferDesc.Height;
}

void UDeviceResources::CopyDepthSRVToPreviousFrameSRV()
{
	if (PreviousFrameDepthSRV)
	{
		PreviousFrameDepthSRV->Release();
		PreviousFrameDepthSRV = nullptr;
	}

	// Create a new texture to copy the current DepthBuffer into
	D3D11_TEXTURE2D_DESC dsDesc = {};
	DepthBuffer->GetDesc(&dsDesc);

	ID3D11Texture2D* newPreviousFrameDepthTexture = nullptr;
	HRESULT hr = Device->CreateTexture2D(&dsDesc, nullptr, &newPreviousFrameDepthTexture);
	if (FAILED(hr))
	{
		// Handle error
		return;
	}

	DeviceContext->CopyResource(newPreviousFrameDepthTexture, DepthBuffer);

	// Create SRV for the new texture
	D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
	ShaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT; // Changed SRV format to R32_FLOAT
	ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ShaderResourceViewDesc.Texture2D.MipLevels = 1;
	hr = Device->CreateShaderResourceView(newPreviousFrameDepthTexture, &ShaderResourceViewDesc, &PreviousFrameDepthSRV);
	if (FAILED(hr))
	{
		// Handle error
		newPreviousFrameDepthTexture->Release();
		return;
	}

	newPreviousFrameDepthTexture->Release(); // Release the texture, the SRV holds a reference
}

void UDeviceResources::OnWindowSizeChanged(uint32 InWidth, uint32 InHeight)
{
	if (InWidth == Width && InHeight == Height)
	{
		return;
	}

	// Ensure width and height are at least 1 to avoid issues with D3D11_TEXTURE2D_DESC
	Width = std::max(1u, InWidth);
	Height = std::max(1u, InHeight);

	// Release all outstanding references to the swap chain's buffers.
	ReleaseFrameBuffer();
	ReleaseDepthBuffer();
	ReleaseSceneColorTargets();

	// Clear the render target and depth stencil views from the device context
	// to ensure no lingering references prevent ResizeBuffers from succeeding.
	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	DeviceContext->Flush(); // Ensure all pending commands are executed

	// Preserve the existing buffer count and format.
	// Automatically choose the width and height to match the client rect for HWNDs.
	HRESULT hr = SwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);

	if (FAILED(hr))
	{
		// Log the error for debugging
		// You might want to assert here or handle the error more robustly
		// For now, just return to prevent further crashes
		return;
	}

	// Recreate the render target view and depth stencil view.
	CreateFrameBuffer();
	CreateDepthBuffer();
	CreateSceneColorTargets();

	// Update the viewport to match the new dimensions.
	UpdateViewport();
}

void UDeviceResources::CreateFactories()
{
	// Direct2D 팩토리 생성
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory);

	// DirectWrite 팩토리 생성
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
	                    reinterpret_cast<IUnknown**>(&DWriteFactory));
}

void UDeviceResources::ReleaseFactories()
{
	if (DWriteFactory)
	{
		DWriteFactory->Release();
		DWriteFactory = nullptr;
	}

	if (D2DFactory)
	{
		D2DFactory->Release();
		D2DFactory = nullptr;
	}
}
