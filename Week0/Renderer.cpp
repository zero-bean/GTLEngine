#include "Renderer.h"


Renderer::Renderer()
{
	ZeroMemory(&ViewportInfo, sizeof(D3D11_VIEWPORT));
}

Renderer::~Renderer()
{
	Release();
}


void Renderer::Initialize(HWND hWindow)
{
    // DeviceAndSwapChain
	CreateDeviceAndSwapChain(hWindow);

    // 파이프라인 셋업
    // 파이프라인 리소스를 설정하고 파이프라인에 박는다.
    // IA, VS, RS, PS, OM 등을 설정한다.
    InitializeAndSetPipeline();

    //INT NumVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
    //CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));
    //SetNumVerticesSphere(NumVerticesSphere);
}

void Renderer::Update(const FVector3& InOffset, float InScale)
{
    UpdateConstant(InOffset, InScale);
}

void Renderer::Render()
{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &offset);

    DeviceContext->Draw(NumVerticesSphere, 0);
}

void Renderer::Release()
{
    ReleaseVertexBuffer();
    ReleaseShader(); // il, vs, ps
    ReleaseRasterizerState(); // rs
    ReleaseFrameBuffer(); // fb, rtv
    ReleaseDeviceAndSwapChain();
}


void Renderer::CreateDeviceAndSwapChain(HWND hWindow)
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

void Renderer::CreateFrameBuffer()
{
    // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

    // 렌더 타겟 뷰 생성
    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
}

void Renderer::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
    rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
}

void Renderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = (sizeof(FConstants) + 0xf) & 0xfffffff0; // 16바이트 정렬
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; 
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    if (!Device)
    {
        OutputDebugStringA("Device is nullptr\n");
    }
    Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
}

void Renderer::CreateShader()
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

void Renderer::SetRenderingPipeline()
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

void Renderer::UpdateConstant(const FVector3& InWorldPosition, float InScale)
{
    D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
    DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
    FConstants* constants = (FConstants*)constantbufferMSR.pData;
    {
        constants->WorldPosition = InWorldPosition;
        constants->Scale = InScale;
    }
    DeviceContext->Unmap(ConstantBuffer, 0);
}

void Renderer::InitializeAndSetPipeline()
{
    // Framebuffer 생성
    CreateFrameBuffer();

    // Rasterizer 생성
    CreateRasterizerState();

    // ConstantBuffer생성
    CreateConstantBuffer();

    // vs, ps, InputLayout 생성
    CreateShader();

    // 파이프라인에 인피니티 스톤박듯이 과정대로 셋
    SetRenderingPipeline();
}

void Renderer::CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
{
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = byteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

    Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &VertexBuffer);
}

void Renderer::SwapBuffer()
{
    SwapChain->Present(1, 0); 
}

void Renderer::Prepare()
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

void Renderer::PrepareShader()
{
    DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
    DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
    DeviceContext->IASetInputLayout(SimpleInputLayout);
}

bool Renderer::ValidateResources()
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

void Renderer::ReleaseVertexBuffer()
{
    VertexBuffer->Release();
    VertexBuffer = nullptr;
}

void Renderer::ReleaseShader()
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

void Renderer::ReleaseRasterizerState()
{
    if (RasterizerState)
    {
        RasterizerState->Release();
        RasterizerState = nullptr;
    }
    // 렌더 타겟을 초기화
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void Renderer::ReleaseFrameBuffer()
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

void Renderer::ReleaseDeviceAndSwapChain()
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


