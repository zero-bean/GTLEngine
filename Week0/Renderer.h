#pragma once
#include "pch.h"


class Renderer
{
public:
    Renderer();
    ~Renderer();

private:
    Renderer(const Renderer&);
    Renderer& operator=(const Renderer&);

public:
    void Initialize(HWND hWindow);
    void Update(const FVector3& InOffset, float InScale);
    void Prepare();
    void PrepareShader();
    void Render();
    void SwapBuffer();

    void Release();


private:
    // Initialize
    void CreateDeviceAndSwapChain(HWND hWindow);

    void InitializeAndSetPipeline();
    void CreateFrameBuffer();
    void CreateRasterizerState();
    void CreateConstantBuffer();
    void CreateShader();
    void SetRenderingPipeline();


    //Update
    void UpdateConstant(const FVector3& InWorldPosition, float InScale);


    // Release
    bool ValidateResources();
    void ReleaseVertexBuffer();
    void ReleaseShader(); // il, vs, ps
    void ReleaseRasterizerState(); // rs
    void ReleaseFrameBuffer(); // fb, rtv
    void ReleaseDeviceAndSwapChain();

    
    
    //버텍스 버퍼는 생성보류
    void CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);

private:
    // 24
    D3D11_VIEWPORT ViewportInfo;

    // 16
    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };

    // 8
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    ID3D11InputLayout* SimpleInputLayout = nullptr;
    ID3D11VertexShader* SimpleVertexShader = nullptr;
    ID3D11RasterizerState* RasterizerState = nullptr;
    ID3D11PixelShader* SimplePixelShader = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
    ID3D11Texture2D* FrameBuffer = nullptr;
    ID3D11RenderTargetView* FrameBufferRTV = nullptr;
   /* ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* ConstantBuffer = nullptr;*/

    // 4
    unsigned int NumVerticesSphere = 0;
    unsigned int Stride = 0;
};

