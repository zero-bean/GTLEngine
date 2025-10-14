#pragma once

#include <d3d11.h>

class UDeviceResources;
class UPipeline;
class FViewport;

class UFXAAPass
{
public:
    UFXAAPass();
    ~UFXAAPass();

    void Initialize(UDeviceResources* InDeviceResources);
    void Release();
    void OnResize();

    ID3D11RenderTargetView* GetSceneRTV() const { return FXAASceneRTV; }
    ID3D11ShaderResourceView* GetSceneSRV() const { return FXAASceneSRV; }

    // Applies FXAA from the offscreen scene texture to the backbuffer across all viewports
    void Apply(UPipeline* InPipeline, FViewport* InViewportManager, ID3D11DepthStencilState* InDisabledDepthState, const FLOAT InClearColor[4]);

private:
    void CreateResources();
    void ReleaseResources();
    void EnsureFixedStates();
    void UpdateFXAAParamsPS(float offsetX, float offsetY, float scaleX, float scaleY);

private:
    UDeviceResources* DeviceResources = nullptr;

    ID3D11Texture2D* FXAASceneTexture = nullptr;
    ID3D11RenderTargetView* FXAASceneRTV = nullptr;
    ID3D11ShaderResourceView* FXAASceneSRV = nullptr;

    ID3D11VertexShader* FXAAVertexShader = nullptr;
    ID3D11PixelShader* FXAAPixelShader = nullptr;
    ID3D11SamplerState* FXAASampler = nullptr;
    ID3D11Buffer* ConstantBufferFXAA = nullptr; // PS slot b0
    ID3D11RasterizerState* RasterState = nullptr; // Solid, Cull None
};

