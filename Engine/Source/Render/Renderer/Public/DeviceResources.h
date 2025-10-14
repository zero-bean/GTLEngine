#pragma once

class UDeviceResources
{
public:
	UDeviceResources(HWND InWindowHandle);
	~UDeviceResources();

	void Create(HWND InWindowHandle);
	void Release();

	void CreateDeviceAndSwapChain(HWND InWindowHandle);
	void ReleaseDeviceAndSwapChain();
	void CreateFrameBuffer();
	void ReleaseFrameBuffer();
	void CreateDepthBuffer();
	void ReleaseDepthBuffer();
	void CreateSceneColorTargets();
	void ReleaseSceneColorTargets();

	// Direct2D/DirectWrite
	void CreateFactories();
	void ReleaseFactories();

	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
	IDXGISwapChain* GetSwapChain() const { return SwapChain; }
	ID3D11RenderTargetView* GetRenderTargetView() const { return FrameBufferRTV; }
	ID3D11DepthStencilView* GetDepthStencilView() const { return DepthStencilView; }
	ID3D11SamplerState* GetDepthSamplerState() const { return DepthSamplerState; }
	ID3D11ShaderResourceView* GetDetphShaderResourceView() const { return DepthShaderResourceView; }
	ID3D11ShaderResourceView* GetPreviousFrameDepthSRV() const { return PreviousFrameDepthSRV; }

	ID3D11Texture2D*           GetSceneColorTexture()  const { return SceneColorTex; }
	ID3D11RenderTargetView*    GetSceneColorRenderTargetView()  const { return SceneColorRTV; }
	ID3D11ShaderResourceView* GetColorShaderResourceView() const { return SceneColorSRV; }

	void CopyDepthSRVToPreviousFrameSRV();
	const D3D11_VIEWPORT& GetViewportInfo() const { return ViewportInfo; }
	void UpdateViewport(float InMenuBarHeight = 0.f);
	void OnWindowSizeChanged(uint32 InWidth, uint32 InHeight);

	// Direct2D/DirectWrite factory getters
	ID2D1Factory* GetD2DFactory() const { return D2DFactory; }
	IDWriteFactory* GetDWriteFactory() const { return DWriteFactory; }

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;

	ID3D11Texture2D* SceneColorTex = nullptr;
	ID3D11RenderTargetView* SceneColorRTV = nullptr;
	ID3D11ShaderResourceView* SceneColorSRV = nullptr;

	ID3D11Texture2D* DepthBuffer = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11SamplerState* DepthSamplerState = nullptr;
	ID3D11ShaderResourceView* DepthShaderResourceView = nullptr;
	ID3D11ShaderResourceView* PreviousFrameDepthSRV = nullptr;

	D3D11_VIEWPORT ViewportInfo = {};

	uint32 Width = 0;
	uint32 Height = 0;

	// Direct2D/DirectWrite factories
	ID2D1Factory* D2DFactory = nullptr;
	IDWriteFactory* DWriteFactory = nullptr;
};
