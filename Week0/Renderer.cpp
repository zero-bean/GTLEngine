#include "Renderer.h"
#include "TextureSet.h"
#include "ScreenUtil.h"

using namespace DirectX;

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

}

void Renderer::Update(const FVector3& InOffset, float InScale, float rotationZDeg)
{
    //UpdateConstant(InOffset, InScale, rotationZDeg);
}

void Renderer::Render(ID3D11Buffer* InConstantBuffer, ID3D11Buffer* InVertexBuffer, UINT numVertices)
{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &InVertexBuffer, &Stride, &offset);
    DeviceContext->VSSetConstantBuffers(0, 1, &InConstantBuffer);
    DeviceContext->Draw(numVertices, 0);
}

void Renderer::Release()
{
    //ReleaseVertexBuffer();
    ReleaseShader(); // il, vs, ps
    ReleaseRasterizerState(); // rs
    ReleaseFrameBuffer(); // fb, rtv
    ReleaseDeviceAndSwapChain();
    ReleaseTextureSampler();
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

//void Renderer::CreateConstantBuffer()
//{
//    D3D11_BUFFER_DESC constantbufferdesc = {};
//    constantbufferdesc.ByteWidth = (sizeof(FConstants) + 0xf) & 0xfffffff0; // 16바이트 정렬
//    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; 
//    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//
//    if (!Device)
//    {
//        OutputDebugStringA("Device is nullptr\n");
//    }
//    Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
//}

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
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    Device->CreateInputLayout(layout, ARRAYSIZE(layout), 
        vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);

    Stride = sizeof(FVertexSimple);

    vertexshaderCSO->Release();
    pixelshaderCSO->Release();
}

void Renderer::CreateTextureSampler()
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;                                                 

    Device->CreateSamplerState(&samplerDesc, &TextureSampler);
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

void Renderer::UpdateConstant(ID3D11Buffer* InConstantBuffer, const FVector3& InWorldPosition, float InScale, float rotationZDeg)
{
    XMMATRIX R = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, XMConvertToRadians(rotationZDeg));

    XMFLOAT4X4 rotationTemp;
    XMStoreFloat4x4(&rotationTemp, XMMatrixTranspose(R));

    D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
    DeviceContext->Map(InConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
    FConstants* constants = (FConstants*)constantbufferMSR.pData;
    {
        constants->WorldPosition = InWorldPosition;
        constants->Scale = InScale;
        constants->rotation = rotationTemp;
        constants->aspectRatio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    }
    DeviceContext->Unmap(InConstantBuffer, 0);
}

void Renderer::InitializeAndSetPipeline()
{
    // Framebuffer 생성
    CreateFrameBuffer();

    // Rasterizer 생성
    CreateRasterizerState();

    // ConstantBuffer생성
    //CreateConstantBuffer();

    // vs, ps, InputLayout 생성
    CreateShader();

    // Texture Sampler 생성
    CreateTextureSampler();

    // 파이프라인에 인피니티 스톤박듯이 과정대로 셋
    SetRenderingPipeline();
}

ID3D11Buffer* Renderer::CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
{
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = byteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

    ID3D11Buffer* vertexBuffer;
    Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

    return vertexBuffer;
}

TextureSet Renderer::LoadTextureSet(const wchar_t* filename)
{
    TextureSet textureSet;

    ID3D11Resource* resource = nullptr;  // 임시 리소스 포인터
    HRESULT hr = CreateWICTextureFromFile(
        Device,
        filename,
        &resource,           // ID3D11Resource**
        &textureSet.srv      // ID3D11ShaderResourceView**
    );

    if (SUCCEEDED(hr) && resource) {
        // 안전하게 Texture2D로 캐스팅
        hr = resource->QueryInterface(__uuidof(ID3D11Texture2D),
            (void**)&textureSet.texture);
        resource->Release(); // 임시 리소스 해제

        if (FAILED(hr)) {
            // 캐스팅 실패 시 정리
            textureSet.Release();
        }
    }

    return textureSet;
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

    // 버텍스 쉐이더에 상수 버퍼를 설정합니다.
    //if (ConstantBuffer)
    //{
    //    DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
    //}
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
    if (!TextureSampler)
    {
        OutputDebugStringA("TextureSampler is nullptr\n");
    }

    bool bResult = Device && DeviceContext && SwapChain && FrameBufferRTV && SimpleInputLayout && 
        SimpleVertexShader && SimplePixelShader && RasterizerState && TextureSampler;

    return bResult;
}

void Renderer::ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
{
    vertexBuffer->Release();
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

void Renderer::ReleaseTextureSampler()
{
    if (TextureSampler)
    {
        TextureSampler->Release();
        TextureSampler = nullptr;
    }
}


