#pragma once
#include "pch.h"



class URenderer
{
public:
    URenderer()
    {
        ZeroMemory(&ViewportInfo, sizeof(D3D11_VIEWPORT));
    }
    ~URenderer()
    {
        Release();
    }
private:
    URenderer(const URenderer&);
    URenderer& operator=(const URenderer&);

public:
    void Initialize(HWND hWindow)
    {
        CreateDeviceAndSwapChain(hWindow);
    }

    void InitializeAndSetPipeline()
    {
        // Framebuffer 생성
        CreateFrameBuffer();

        // Rasterizer 생성
        CreateRasterizerState();

        // vs, ps, InputLayout 생성
        CreateShader();

        // 파이프라인에 인피니티 스톤박듯이 과정대로 셋
        SetRenderingPipeline();
    }

    void Update(const FVector3& InOffset, float InScale, ID3D11Buffer* const InConstantBuffer)
    {
        UpdateConstant(InOffset, InScale, InConstantBuffer);
    }
    void Render()
    {
        UINT offset = 0;
        DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &offset);

        DeviceContext->Draw(NumVerticesSphere, 0);
    }

    // update
    void UpdateConstant(const FVector3& InWorldPosition, float InScale, ID3D11Buffer* const InConstantBuffer)
    {
        if (!InConstantBuffer)
        {
            OutputDebugStringA("InConstantBuffer is nullptr\n");
            return;
        }

        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(InConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
        FConstants* constants = (FConstants*)constantbufferMSR.pData;
        {
            constants->WorldPosition = InWorldPosition;
            constants->Scale = InScale;
        }
        DeviceContext->Unmap(InConstantBuffer, 0);
    }

    void SetConstantBuffer(ID3D11Buffer* const InConstantBuffer)
    {
        // VS Stage에 각 공마다 설정된 ConstantBuffer 설정
        if (!InConstantBuffer)
        {
            OutputDebugStringA("InConstantBuffer is nullptr\n");
            return;
        }
        DeviceContext->VSSetConstantBuffers(0, 1, &InConstantBuffer);
    }

    void CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
    {
        // 2. Create a vertex buffer
        D3D11_BUFFER_DESC vertexbufferdesc = {};
        vertexbufferdesc.ByteWidth = byteWidth;
        vertexbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
        vertexbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

        Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &VertexBuffer);
    }

    void SwapBuffer()
    {
        SwapChain->Present(1, 0); // 1: VSync 활성화
    }

    void Prepare()
    {
        // Clear
        DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

        DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // RS
        DeviceContext->RSSetViewports(1, &ViewportInfo);
        DeviceContext->RSSetState(RasterizerState);

        // OM
        DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
        DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    }


    void PrepareShader()
    {
        DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
        DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
        DeviceContext->IASetInputLayout(SimpleInputLayout);
    }

    // getter
    inline ID3D11Device* GetDevice() const { return Device; }
    inline ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
    inline ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    inline unsigned int         GetNumVerticesSphere() const { return NumVerticesSphere; }

    // setter
    inline void SetNumVerticesSphere(unsigned int InNumVerticesSphere) { NumVerticesSphere = InNumVerticesSphere; }


    void Release()
    {
        /*
        디바이스, 디컨, 스왑, 뷰포트
        프버, RTV, 래스터, vs,ps,il
        버텍스 버퍼
        */
        ReleaseVertexBuffer();
        ReleaseShader(); // il, vs, ps
        ReleaseRasterizerState(); // rs
        ReleaseFrameBuffer(); // fb, rtv
        ReleaseDeviceAndSwapChain();
    }

private:
    // Initialize
    void CreateDeviceAndSwapChain(HWND hWindow)
    {
        // 지원하는 Direct3D 기능 레벨을 정의
        D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

        // 스왑 체인 설정 구조체 초기화
        DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
        swapchaindesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
        swapchaindesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
        swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
        swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
        swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
        swapchaindesc.BufferCount = 2; // 더블 버퍼링
        swapchaindesc.OutputWindow = hWindow; // 렌더링할 창 핸들
        swapchaindesc.Windowed = TRUE; // 창 모드
        swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

        // Direct3D 장치와 스왑 체인을 생성
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
            featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
            &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

        // 생성된 스왑 체인의 정보 가져오기
        SwapChain->GetDesc(&swapchaindesc);

        // 뷰포트 정보 설정
        ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
    }

    // Initialize Pipeline Resource
    void CreateFrameBuffer()
    {
        // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
        SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

        // 렌더 타겟 뷰 생성
        D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
        framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
        framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

        Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
    }
    void CreateRasterizerState()
    {
        D3D11_RASTERIZER_DESC rasterizerdesc = {};
        rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
        rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

        Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
    }
    void CreateShader()
    {
        ID3DBlob* vertexshaderCSO;
        ID3DBlob* pixelshaderCSO;

        D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);

        Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

        D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCSO, nullptr);

        Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);

        Stride = sizeof(FVertexSimple);

        vertexshaderCSO->Release();
        pixelshaderCSO->Release();
    }
    bool ValidateResources() const
    {
        // Set PipiLine Resource
        if (!Device)
        {
            OutputDebugStringA("Device is nullptr\n");
        }
        if (!DeviceContext)
        {
            OutputDebugStringA("DeviceContext is nullptr\n");
        }
        if (!SwapChain)
        {
            OutputDebugStringA("SwapChain is nullptr\n");
        }
        if (!FrameBufferRTV)
        {
            OutputDebugStringA("RTV is nullptr\n");
        }

        // Set PipeLine-Stage
        if (!SimpleInputLayout)
        {
            OutputDebugStringA("SimpleInputLayout is nullptr\n");
        }

        if (!SimpleVertexShader)
        {
            OutputDebugStringA("SimpleVertexShader is nullptr\n");
        }

        if (!SimplePixelShader)
        {
            OutputDebugStringA("SimplePixelShader is nullptr\n");
        }
        if (!RasterizerState)
        {
            OutputDebugStringA("RasterizerState is nullptr\n");
        }

        bool bResult = Device && DeviceContext && SwapChain && FrameBufferRTV && SimpleInputLayout && SimpleVertexShader && SimplePixelShader && RasterizerState;

        return bResult;
    }

    void SetRenderingPipeline()
    {
        if (ValidateResources())
        {
            // Clear
            DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

            // IA
            DeviceContext->IASetInputLayout(SimpleInputLayout);
            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // VS
            DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);

            // RS
            DeviceContext->RSSetViewports(1, &ViewportInfo);
            DeviceContext->RSSetState(RasterizerState);

            // PS
            DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);

            // OM
            DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
            DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
        }
    }

    // Release
    void ReleaseDeviceAndSwapChain()
    {
        if (DeviceContext)
        {
            DeviceContext->Flush(); // 남아있는 GPU 명령 실행
        }

        if (SwapChain)
        {
            SwapChain->Release();
            SwapChain = nullptr;
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
    void ReleaseRasterizerState()
    {
        if (RasterizerState)
        {
            RasterizerState->Release();
            RasterizerState = nullptr;
        }
        // 렌더 타겟을 초기화
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }
    void ReleaseFrameBuffer()
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
    void ReleaseShader()
    {
        if (SimpleInputLayout)
        {
            SimpleInputLayout->Release();
            SimpleInputLayout = nullptr;
        }

        if (SimplePixelShader)
        {
            SimplePixelShader->Release();
            SimplePixelShader = nullptr;
        }

        if (SimpleVertexShader)
        {
            SimpleVertexShader->Release();
            SimpleVertexShader = nullptr;
        }
    }
    void ReleaseVertexBuffer()
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }

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
    ID3D11Buffer* VertexBuffer = nullptr;

    // 4
    unsigned int NumVerticesSphere = 0;
    unsigned int Stride = 0;
};
