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

	ID3D11Device* GetDevice() { return Device; }
	ID3D11DeviceContext* GetDeviceContext() { return DeviceContext; }
	IDXGISwapChain* GetSwapChain() { return SwapChain; }
	ID3D11RenderTargetView* GetRenderTargetView() { return FrameBufferRTV; }

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;

};

