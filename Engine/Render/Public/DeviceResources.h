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

	ID3D11Device* GetDevice() { return Device; }
	ID3D11DeviceContext* GetDeviceContext() { return DeviceContext; }
	IDXGISwapChain* GetSwapChain() { return SwapChain; }
	ID3D11RenderTargetView* GetRenderTargetView() { return FrameBufferRTV; }
	ID3D11DepthStencilView* GetDepthStencilView() {return DepthStencilView; }
	D3D11_VIEWPORT& GetViewportInfo() { return ViewportInfo; }

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;

	ID3D11Texture2D* DepthBuffer = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;

	D3D11_VIEWPORT ViewportInfo = { 0 };

	UINT Width = 0;
	UINT Height = 0;
};
