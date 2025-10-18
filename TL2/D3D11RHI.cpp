#include "pch.h"
#include "UI/StatsOverlayD2D.h"
#include "D3D11RHI.h"

void D3D11RHI::Initialize(HWND hWindow)
{
    // 이곳에서 Device, DeviceContext, viewport, swapchain를 초기화한다
    CreateDeviceAndSwapChain(hWindow);
    CreateFrameBuffer();
    CreateRasterizerState();
    CreateBlendState();
    CreateConstantBuffer();
	CreateDepthStencilState();
	CreateSamplerState();
    CreateIdBuffer();

    CreateScreenTexture(&TemporalBuffer);
    CreateRTV(TemporalBuffer, &TemporalRTV);
    CreateSRV(TemporalBuffer, &TemporalSRV);

    UResourceManager::GetInstance().Initialize(Device,DeviceContext);

    // Initialize Direct2D overlay after device/swapchain ready
    UStatsOverlayD2D::Get().Initialize(Device, DeviceContext, SwapChain);
}

void D3D11RHI::Release()
{
    if (DeviceContext)
    {
        // 파이프라인에서 바인딩된 상태/리소스를 명시적으로 해제
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        DeviceContext->OMSetDepthStencilState(nullptr, 0);
        DeviceContext->RSSetState(nullptr);
        DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

        DeviceContext->ClearState();
        DeviceContext->Flush();
    }

    ReleaseSamplerState();

    // 상수버퍼
    CBUFFER_TYPE_LIST(RELEASE_CBUFFER)


    // 상태 객체
    if (DepthStencilState) { DepthStencilState->Release(); DepthStencilState = nullptr; }
    if (DepthStencilStateLessEqualWrite) { DepthStencilStateLessEqualWrite->Release(); DepthStencilStateLessEqualWrite = nullptr; }
    if (DepthStencilStateLessEqualReadOnly) { DepthStencilStateLessEqualReadOnly->Release(); DepthStencilStateLessEqualReadOnly = nullptr; }
    if (DepthStencilStateAlwaysNoWrite) { DepthStencilStateAlwaysNoWrite->Release(); DepthStencilStateAlwaysNoWrite = nullptr; }
    if (DepthStencilStateDisable) { DepthStencilStateDisable->Release(); DepthStencilStateDisable = nullptr; }
    if (DepthStencilStateGreaterEqualWrite) { DepthStencilStateGreaterEqualWrite->Release(); DepthStencilStateGreaterEqualWrite = nullptr; }

    if (DefaultRasterizerState) { DefaultRasterizerState->Release();   DefaultRasterizerState = nullptr; }
    if (WireFrameRasterizerState) { WireFrameRasterizerState->Release();   WireFrameRasterizerState = nullptr; }
    if (NoCullRasterizerState) { NoCullRasterizerState->Release();   NoCullRasterizerState = nullptr; }
    if (FrontCullRasterizerState) { FrontCullRasterizerState->Release();   FrontCullRasterizerState = nullptr; }
    if (DecalRasterizerState) { DecalRasterizerState->Release();   DecalRasterizerState = nullptr; }

    ReleaseTexture(&IdBuffer, &IdBufferRTV, nullptr);
    ReleaseTexture(&IdStagingBuffer, nullptr, nullptr);
    ReleaseTexture(&FrameBuffer, &FrameRTV, &FrameSRV);
    ReleaseTexture(&TemporalBuffer, &TemporalRTV, &TemporalSRV);
    ReleaseDepthStencilView(&DepthStencilView, &DepthSRV);

    ReleaseBlendState();
    ReleaseDeviceAndSwapChain();
}

void D3D11RHI::ClearBackBuffer()
{
    float ClearColor[4] = { 0.10f, 0.10f, 0.10f, 1.0f };
    DeviceContext->ClearRenderTargetView(FrameRTV, ClearColor);
    float IDColor[4] = { 0.0f,0.0f,0.0f,0.0f };
    DeviceContext->ClearRenderTargetView(IdBufferRTV, IDColor);
}

void D3D11RHI::ClearDepthBuffer(float Depth, UINT Stencil)
{
    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, Depth, Stencil);

}

void D3D11RHI::CreateBlendState()
{
    D3D11_BLEND_DESC BlendDesc = {};
 
    BlendDesc.IndependentBlendEnable = TRUE;

    D3D11_RENDER_TARGET_BLEND_DESC& Rt0 = BlendDesc.RenderTarget[0];

    Rt0.BlendEnable = TRUE;
    Rt0.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    Rt0.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    Rt0.BlendOp = D3D11_BLEND_OP_ADD;
    Rt0.SrcBlendAlpha = D3D11_BLEND_ONE;
    Rt0.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    Rt0.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    Rt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;


    D3D11_RENDER_TARGET_BLEND_DESC& Rt1 = BlendDesc.RenderTarget[1];
    Rt1.BlendEnable = FALSE;
    Rt1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Device->CreateBlendState(&BlendDesc, &BlendStateTransparent);

    BlendDesc = {};
    BlendDesc.IndependentBlendEnable = TRUE;
    Rt0 = BlendDesc.RenderTarget[0];
    Rt0.BlendEnable = FALSE;
    Rt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Rt1 = BlendDesc.RenderTarget[1];
    Rt1.BlendEnable = FALSE;
    Rt1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    Device->CreateBlendState(&BlendDesc, &BlendStateOpaque);
}

void D3D11RHI::CreateDepthStencilState()
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.StencilEnable = FALSE;

    // 1) 기본: LessEqual + Write ALL
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    Device->CreateDepthStencilState(&desc, &DepthStencilStateLessEqualWrite);

    // 2) ReadOnly: LessEqual + Write ZERO
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    Device->CreateDepthStencilState(&desc, &DepthStencilStateLessEqualReadOnly);

    // 3) AlwaysNoWrite: Always + Write ZERO (기즈모/오버레이 용)
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    // DepthEnable은 TRUE 유지 (읽기 의미는 없지만 상태 일관성을 위해)
    Device->CreateDepthStencilState(&desc, &DepthStencilStateAlwaysNoWrite);

    // 4) Disable: DepthEnable FALSE (테스트/쓰기 모두 무시)
    desc.DepthEnable = FALSE;
    // DepthWriteMask/Func는 무시되지만 값은 그대로 둬도 됨
    Device->CreateDepthStencilState(&desc, &DepthStencilStateDisable);

    // 5) (선택) GreaterEqual + Write ALL
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    Device->CreateDepthStencilState(&desc, &DepthStencilStateGreaterEqualWrite);
}

void D3D11RHI::CreateSamplerState()
{
    D3D11_SAMPLER_DESC SampleDesc = {};
    SampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SampleDesc.MinLOD = 0;
    SampleDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT HR = Device->CreateSamplerState(&SampleDesc, &DefaultSamplerState);
}

void D3D11RHI::CreateIdBuffer()
{
    CreateScreenTexture(&IdBuffer, DXGI_FORMAT_R32_UINT);
    D3D11_RENDER_TARGET_VIEW_DESC Desc{};
    Desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    Desc.Format = DXGI_FORMAT_R32_UINT;
    CreateRTV(IdBuffer, &IdBufferRTV, Desc);

    D3D11_TEXTURE2D_DESC TextureDesc{};
    TextureDesc.Format = DXGI_FORMAT_R32_UINT;
    TextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    TextureDesc.Usage = D3D11_USAGE_STAGING;
    TextureDesc.Width = 1;
    TextureDesc.Height = 1;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.BindFlags = 0;
    CreateTexture2D(TextureDesc, &IdStagingBuffer);
}

HRESULT D3D11RHI::CreateIndexBuffer(ID3D11Device* device, const FMeshData* meshData, ID3D11Buffer** outBuffer)
{
    if (!meshData || meshData->Indices.empty())
        return E_FAIL;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(sizeof(uint32) * meshData->Indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = meshData->Indices.data();

    return device->CreateBuffer(&ibd, &iinitData, outBuffer);
}

HRESULT D3D11RHI::CreateIndexBuffer(ID3D11Device* device, const FStaticMesh* mesh, ID3D11Buffer** outBuffer)
{
    if (!mesh || mesh->Indices.empty())
        return E_FAIL;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(sizeof(uint32) * mesh->Indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = mesh->Indices.data();

    return device->CreateBuffer(&ibd, &iinitData, outBuffer);
}

void D3D11RHI::IASetPrimitiveTopology()
{
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void D3D11RHI::RSSetState(EViewModeIndex ViewModeIndex)
{
    if (ViewModeIndex == EViewModeIndex::VMI_Wireframe)
    {
        DeviceContext->RSSetState(WireFrameRasterizerState);
    }
    else
    {
        DeviceContext->RSSetState(DefaultRasterizerState);
    }
}

void D3D11RHI::RSSetFrontCullState()
{
    DeviceContext->RSSetState(FrontCullRasterizerState);
}

void D3D11RHI::RSSetNoCullState()
{
    DeviceContext->RSSetState(NoCullRasterizerState);
}

void D3D11RHI::RSSetDefaultState()
{
    DeviceContext->RSSetState(DefaultRasterizerState);
}

void D3D11RHI::RSSetDecalState()
{
    DeviceContext->RSSetState(DecalRasterizerState);
}

void D3D11RHI::RSSetViewport()
{
    DeviceContext->RSSetViewports(1, &ViewportInfo);
}

void D3D11RHI::OMSetDepthOnlyTarget()
{
    DeviceContext->OMSetRenderTargets(0, nullptr, DepthStencilView);
}

//수정 필요
//postprocessing 이 RenderTarget을 직접 참조하고 원하는 렌더타겟을 설정 할 수 있어야 하나,
//지금 발제는 처리가능하기에 일단 이대로 둠
void D3D11RHI::OMSetRenderTargets(const ERenderTargetType RenderTargetType)
{

    TArray<ID3D11RenderTargetView*> RTVList;
    ID3D11DepthStencilView* Depth = DepthStencilView;
    if (((int)RenderTargetType & (int)ERenderTargetType::None) > 0)
    {
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        return;
    }

    if (((int)RenderTargetType & (int)ERenderTargetType::Frame) > 0)
    {
        RTVList.Push(FrameRTV);
    }
    if (((int)RenderTargetType & (int)ERenderTargetType::ID) > 0)
    {
        RTVList.Push(IdBufferRTV);
    }
    if (((int)RenderTargetType & (int)ERenderTargetType::Temporal) > 0)
    {
        RTVList.Push(TemporalRTV);
    }
    //depth를 SRV로 안 써야 RTV로 설정 가능
    if (((int)RenderTargetType & (int)ERenderTargetType::NoDepth) > 0)
    {
        Depth = nullptr;
    }
   
    DeviceContext->OMSetRenderTargets(RTVList.size(), RTVList.data(), Depth);
}

//수정 필요
void D3D11RHI::PSSetRenderTargetSRV(const ERenderTargetType RenderTargetType)
{
    D3D11_TEXTURE2D_DESC desc{};
    FrameBuffer->GetDesc(&desc);
    D3D11_TEXTURE2D_DESC desc2{};
    TemporalBuffer->GetDesc(&desc2);
    TArray< ID3D11ShaderResourceView*> SRVList;
    D3D11_VIEWPORT ViewPorts;
    UINT count = 0;

    /*D3D11_VIEWPORT ViewPort;
    ViewPort.TopLeftX = 0;
    ViewPort.TopLeftY = 0;
    ViewPort.Width = desc.Width * 0.5f;
    ViewPort.Height = desc.Height;
    ViewPort.MinDepth = 0;
    ViewPort.MaxDepth = 1;
    
   DeviceContext->RSSetViewports(1, &ViewPort);*/
    if (((int)RenderTargetType & (int)ERenderTargetType::None) > 0)
    {
        //현재까지 최대 2개
        ID3D11ShaderResourceView* Empty[2]{ nullptr, nullptr };
        DeviceContext->PSSetShaderResources(0, 2, Empty);
        return;
    }
    if (((int)RenderTargetType & (int)ERenderTargetType::Frame) > 0)
    {
        SRVList.Add(FrameSRV);
    }
    else if (((int)RenderTargetType & (int)ERenderTargetType::Temporal) > 0)
    {
        SRVList.Add(TemporalSRV);
    }
    if (((int)RenderTargetType & (int)ERenderTargetType::Depth) > 0)
    {
        SRVList.Add(DepthSRV);
    }
    DeviceContext->PSSetShaderResources(0, SRVList.size(), SRVList.data());
}

void D3D11RHI::CreateVertexBuffer(ID3D11Device* device, const TArray<FVertexUV> Vertices, ID3D11Buffer** outBuffer)
{ 
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;             // GPU에서만 읽고, CPU는 접근하지 않음
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;    // 버텍스 버퍼로 사용
    desc.ByteWidth = sizeof(FVertexUV) * Vertices.size();// 전체 버퍼 크기
    desc.CPUAccessFlags = 0;                      // CPU 접근 불가
    desc.MiscFlags = 0;                           // 기타 옵션 없음
    desc.StructureByteStride = 0;                 // StructuredBuffer가 아니므로 0

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = Vertices.data();
    device->CreateBuffer(&desc, &initData, outBuffer);
}


void D3D11RHI::OMSetBlendState(bool bIsBlendMode)
{
    if (bIsBlendMode == true)
    {
        DeviceContext->OMSetBlendState(BlendStateTransparent, nullptr, 0xffffffff);
    }
    else
    {
        DeviceContext->OMSetBlendState(BlendStateOpaque, nullptr, 0xffffffff);
    }
}

void D3D11RHI::Present()
{
    // Draw any Direct2D overlays before present
    UStatsOverlayD2D::Get().Draw();
    SwapChain->Present(1, 0); // vsync on
}

void D3D11RHI::CreateScreenTexture(ID3D11Texture2D** Texture, DXGI_FORMAT Format)
{
    DXGI_SWAP_CHAIN_DESC SwapDesc;
    SwapChain->GetDesc(&SwapDesc);

    D3D11_TEXTURE2D_DESC TextureDesc{};
    TextureDesc.Format = Format;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.Width = SwapDesc.BufferDesc.Width;
    TextureDesc.Height = SwapDesc.BufferDesc.Height;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    CreateTexture2D(TextureDesc, Texture);
}

void D3D11RHI::CreateTexture2D(D3D11_TEXTURE2D_DESC& Desc, ID3D11Texture2D** Texture)
{
    HRESULT hr = Device->CreateTexture2D(&Desc, nullptr, Texture);
    if (FAILED(hr))
    {
        int a = 0;
    }
}
void D3D11RHI::CreateDepthStencilView(ID3D11DepthStencilView** DSV, ID3D11ShaderResourceView** SRV)
{
    DXGI_SWAP_CHAIN_DESC swapDesc;
    SwapChain->GetDesc(&swapDesc);

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = swapDesc.BufferDesc.Width;
    depthDesc.Height = swapDesc.BufferDesc.Height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // SRV를 생성하려면 TYPELESS 포맷 사용
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // SRV 바인딩 추가

    // DepthStencilView 생성
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DSV는 D24_UNORM_S8_UINT 포맷
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    ID3D11Texture2D* depthBuffer = nullptr;
    CreateTexture2D(depthDesc, &depthBuffer);
    HRESULT hr = Device->CreateDepthStencilView(depthBuffer, &dsvDesc, DSV);
    if (FAILED(hr))
    {
        int a = 0;
    }
    // ShaderResourceView 생성 (데칼 렌더링에서 사용)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // SRV는 R24_UNORM_X8_TYPELESS 포맷
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    CreateSRV(depthBuffer, SRV, srvDesc);

    depthBuffer->Release(); // 뷰만 참조 유지
}

void D3D11RHI::CreateSRV(ID3D11Texture2D* Resource, ID3D11ShaderResourceView** SRV)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC Desc{};
    Desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    Desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    Desc.Texture2D.MipLevels = 1;
    CreateSRV(Resource, SRV, Desc);
}
void D3D11RHI::CreateRTV(ID3D11Texture2D* Resource, ID3D11RenderTargetView** RTV)
{
    D3D11_RENDER_TARGET_VIEW_DESC Desc{};
    Desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    Desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    CreateRTV(Resource, RTV, Desc);
}
void D3D11RHI::CreateSRV(ID3D11Texture2D* Resource, ID3D11ShaderResourceView** SRV, D3D11_SHADER_RESOURCE_VIEW_DESC& Desc)
{
    HRESULT hr = Device->CreateShaderResourceView(Resource, &Desc, SRV);
    if (FAILED(hr))
    {
        int a = 0;
    }
}
void D3D11RHI::CreateRTV(ID3D11Texture2D* Resource, ID3D11RenderTargetView** RTV, D3D11_RENDER_TARGET_VIEW_DESC& Desc)
{
    HRESULT hr = Device->CreateRenderTargetView(Resource, &Desc, RTV);
    if (FAILED(hr))
    {
        int a = 0;
    }
}
void D3D11RHI::CreateDeviceAndSwapChain(HWND hWindow)
{
    // 지원하는 Direct3D 기능 레벨을 정의
    D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // 스왑 체인 설정 구조체 초기화
    DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
    swapchaindesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
    swapchaindesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
    swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
    swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
    swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT; // 렌더 타겟으로 사용
    swapchaindesc.BufferCount = 2; // 더블 버퍼링
    swapchaindesc.OutputWindow = hWindow; // 렌더링할 창 핸들
    swapchaindesc.Windowed = TRUE; // 창 모드
    swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

    // Direct3D 장치와 스왑 체인을 생성
    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags,
        featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
        &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);
    // 생성된 스왑 체인의 정보 가져오기
    SwapChain->GetDesc(&swapchaindesc);

    // 뷰포트 정보 설정
    ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
}

void D3D11RHI::CreateFrameBuffer()
{
    // 백 버퍼 가져오기
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

    CreateRTV(FrameBuffer, &FrameRTV);
    CreateSRV(FrameBuffer, &FrameSRV);

    CreateDepthStencilView(&DepthStencilView, &DepthSRV);
}

void D3D11RHI::CreateRasterizerState()
{
    // 이미 생성된 경우 중복 생성 방지
    if (DefaultRasterizerState)
        return;

    D3D11_RASTERIZER_DESC deafultrasterizerdesc = {};
    deafultrasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
    deafultrasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링
    deafultrasterizerdesc.DepthClipEnable = TRUE; // 근/원거리 평면 클리핑

    Device->CreateRasterizerState(&deafultrasterizerdesc, &DefaultRasterizerState);

    D3D11_RASTERIZER_DESC wireframerasterizerdesc = {};
    wireframerasterizerdesc.FillMode = D3D11_FILL_WIREFRAME; // 채우기 모드
    wireframerasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링
    wireframerasterizerdesc.DepthClipEnable = TRUE; // 근/원거리 평면 클리핑

    Device->CreateRasterizerState(&wireframerasterizerdesc, &WireFrameRasterizerState);

    D3D11_RASTERIZER_DESC frontcullrasterizerdesc = {};
    frontcullrasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
    frontcullrasterizerdesc.CullMode = D3D11_CULL_FRONT; // 프론트 페이스 컬링
    frontcullrasterizerdesc.DepthClipEnable = TRUE; // 근/원거리 평면 클리핑

    Device->CreateRasterizerState(&frontcullrasterizerdesc, &FrontCullRasterizerState);

    D3D11_RASTERIZER_DESC nocullrasterizerdesc = {};
    nocullrasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
    nocullrasterizerdesc.CullMode = D3D11_CULL_NONE; // 컬링 없음
    nocullrasterizerdesc.DepthClipEnable = TRUE; // 근/원거리 평면 클리핑

    Device->CreateRasterizerState(&nocullrasterizerdesc, &NoCullRasterizerState);

    // 데칼용 DepthBias 적용된 state
    D3D11_RASTERIZER_DESC decalrasterizerdesc = {};
    decalrasterizerdesc.FillMode = D3D11_FILL_SOLID;
    decalrasterizerdesc.CullMode = D3D11_CULL_NONE;
    decalrasterizerdesc.DepthClipEnable = TRUE;
    decalrasterizerdesc.DepthBias = -100; // z-fighting 방지용 bias
    decalrasterizerdesc.DepthBiasClamp = 0.0f;
    decalrasterizerdesc.SlopeScaledDepthBias = -1.0f;

    Device->CreateRasterizerState(&decalrasterizerdesc, &DecalRasterizerState);
}

void D3D11RHI::CreateConstantBuffer()
{
    CBUFFER_TYPE_LIST(CREATE_CBUFFER)
}

void D3D11RHI::ReleaseSamplerState()
{
    if (DefaultSamplerState)
    {
        DefaultSamplerState->Release();
        DefaultSamplerState = nullptr;
	}
}

void D3D11RHI::ReleaseBlendState()
{
    if (BlendStateOpaque) { BlendStateOpaque->Release();        BlendStateOpaque = nullptr; }
    if (BlendStateTransparent) { BlendStateTransparent->Release();  BlendStateTransparent = nullptr; }
}

void D3D11RHI::ReleaseRasterizerState()
{
    if (DefaultRasterizerState)
    {
        DefaultRasterizerState->Release();
        DefaultRasterizerState = nullptr;
    }
    if (WireFrameRasterizerState)
    {
        WireFrameRasterizerState->Release();
        WireFrameRasterizerState = nullptr;
    }
    if (FrontCullRasterizerState)
    {
        FrontCullRasterizerState->Release();
        FrontCullRasterizerState = nullptr;
    }
    if (NoCullRasterizerState)
    {
        NoCullRasterizerState->Release();
        NoCullRasterizerState = nullptr;
    }
    if (DecalRasterizerState)
    {
        DecalRasterizerState->Release();
        DecalRasterizerState = nullptr;
    }
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void D3D11RHI::ReleaseTexture(ID3D11Texture2D** Texture, ID3D11RenderTargetView** RTV, ID3D11ShaderResourceView** SRV)
{
    if (RTV && (*RTV))
    {
        (*RTV)->Release();
        (*RTV) = nullptr;
    }
    if (SRV && (*SRV))
    {
        (*SRV)->Release();
        (*SRV) = nullptr;
    }
    if (Texture && (*Texture))
    {
        (*Texture)->Release();
        (*Texture) = nullptr;
    }
}
void D3D11RHI::ReleaseDepthStencilView(ID3D11DepthStencilView** DSV, ID3D11ShaderResourceView** SRV)
{
    if (SRV &&(*SRV))
    {
        (*SRV)->Release();
        (*SRV) = nullptr;
    }
    if (DSV && (*DSV))
    {
        (*DSV)->Release();
        (*DSV) = nullptr;
    }
}

void D3D11RHI::ReleaseDeviceAndSwapChain()
{
    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }

}

void D3D11RHI::OmSetDepthStencilState(EComparisonFunc Func)
{
    switch (Func)
    {
    case EComparisonFunc::Always:
        DeviceContext->OMSetDepthStencilState(DepthStencilStateAlwaysNoWrite, 0);
        break;
    case EComparisonFunc::LessEqual:
        DeviceContext->OMSetDepthStencilState(DepthStencilStateLessEqualWrite, 0);
        break;
    case EComparisonFunc::LessEqualReadOnly:
        DeviceContext->OMSetDepthStencilState(DepthStencilStateLessEqualReadOnly, 0);
        break;
    case EComparisonFunc::GreaterEqual:
        DeviceContext->OMSetDepthStencilState(DepthStencilStateGreaterEqualWrite, 0);
        break;
    case EComparisonFunc::Disable:
        DeviceContext->OMSetDepthStencilState(DepthStencilStateDisable, 0);
    }
}

// ──────────────────────────────────────────────────────
// Helper: Viewport 갱신
// ──────────────────────────────────────────────────────
void D3D11RHI::SetViewport(UINT width, UINT height)
{
    ViewportInfo.TopLeftX = 0.0f;
    ViewportInfo.TopLeftY = 0.0f;
    ViewportInfo.Width = static_cast<float>(width);
    ViewportInfo.Height = static_cast<float>(height);
    ViewportInfo.MinDepth = 0.0f;
    ViewportInfo.MaxDepth = 1.0f;

    DeviceContext->RSSetViewports(1, &ViewportInfo);
}

void D3D11RHI::ResizeSwapChain(UINT width, UINT height)
{
    if (!SwapChain) return;

    // 렌더링 완료까지 대기 (중요!)
    if (DeviceContext) {
        DeviceContext->Flush();
    }

    // 현재 렌더 타겟 언바인딩
    if (DeviceContext) {
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }
    // 기존 뷰 해제
    ReleaseTexture(&IdStagingBuffer, nullptr, nullptr);
    ReleaseTexture(&IdBuffer, &IdBufferRTV, nullptr);
    ReleaseTexture(&FrameBuffer, &FrameRTV, &FrameSRV);
    ReleaseTexture(&TemporalBuffer, &TemporalRTV, &TemporalSRV);
    ReleaseDepthStencilView(&DepthStencilView, &DepthSRV);

    // 스왑체인 버퍼 리사이즈
    HRESULT hr = SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) 
    { 
        UE_LOG("ResizeBuffers failed!\n");
        return; 
    } 

    // 1) 백버퍼에서 RTV 생성
    hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);
    if (FAILED(hr)) {
        UE_LOG("GetBuffer(0) failed.\n");
        return;
    }

    CreateRTV(FrameBuffer, &FrameRTV);
    CreateSRV(FrameBuffer, &FrameSRV);

    CreateScreenTexture(&TemporalBuffer);
    CreateRTV(TemporalBuffer, &TemporalRTV);
    CreateSRV(TemporalBuffer, &TemporalSRV);

    CreateDepthStencilView(&DepthStencilView, &DepthSRV);
    CreateIdBuffer();
    // 뷰포트도 갱신
    SetViewport(width, height);
}

void D3D11RHI::PSSetDefaultSampler(UINT StartSlot)
{
	DeviceContext->PSSetSamplers(StartSlot, 1, &DefaultSamplerState);
}
