#include <windows.h>
#include <crtdbg.h>  

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")


#include <d3d11.h>
#include <d3dcompiler.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"


// 코딩규칙 
//struct 는 F를 붙힌다




#include "pch.h"
//using namespace DirectX;
#include "Sphere.h"

// 파란색 RGBA
#define BLUE_R 0.0f
#define BLUE_G 0.0f
#define BLUE_B 1.0f
#define BLUE_A 1.0f

FVertexSimple arrowVertices[] =
{
    // 몸통 (직사각형) - 시계 방향
    {  0.05f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A },
    {  0.05f, -0.2f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A },
    { -0.05f, -0.2f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A },


    { -0.05f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A },
    {  0.05f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A },
    { -0.05f, -0.2f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A },

    // 화살표 머리 (삼각형) - 시계 방향
    {  0.0f,  0.6f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A },
    {  0.1f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A },
    { -0.1f,  0.4f, 0.0f,  BLUE_R, BLUE_G, BLUE_B, BLUE_A },
};

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

    void Update(const FVector3& InOffset, float InScale, float rotationZDeg, ID3D11Buffer* const InConstantBuffer)
    {
        UpdateConstant(InOffset, InScale, rotationZDeg, InConstantBuffer);
    }
    void Render()
    {
        UINT offset = 0;
        DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &offset);

        DeviceContext->Draw(NumVerticesSphere, 0);
    }

    // update
    void UpdateConstant(const FVector3& InWorldPosition, float InScale, float rotationZDeg, ID3D11Buffer* const InConstantBuffer)
    {
        if (!InConstantBuffer)
        {
            OutputDebugStringA("InConstantBuffer is nullptr\n");
            return;
        }

        DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(0.0f, 0.0f, DirectX::XMConvertToRadians(rotationZDeg));

        DirectX::XMFLOAT4X4 rotationTemp;
        XMStoreFloat4x4(&rotationTemp, XMMatrixTranspose(R));

        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(InConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
        FConstants* constants = (FConstants*)constantbufferMSR.pData;
        {
            constants->WorldPosition = InWorldPosition;
            constants->Scale = InScale;
            constants->rotation = rotationTemp;
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
    inline ID3D11Device*        GetDevice() const               { return Device; }
    inline ID3D11DeviceContext* GetDeviceContext() const        { return DeviceContext; }
    inline ID3D11Buffer*        GetVertexBuffer() const         { return VertexBuffer; }
    inline unsigned int         GetNumVerticesSphere() const    { return NumVerticesSphere; }

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
    ID3D11Device* Device                           = nullptr;
    ID3D11DeviceContext* DeviceContext             = nullptr;
    ID3D11InputLayout* SimpleInputLayout           = nullptr;
    ID3D11VertexShader* SimpleVertexShader         = nullptr;
    ID3D11RasterizerState* RasterizerState         = nullptr;
    ID3D11PixelShader* SimplePixelShader           = nullptr;
    IDXGISwapChain* SwapChain                      = nullptr;
    ID3D11Texture2D* FrameBuffer                   = nullptr;
    ID3D11RenderTargetView* FrameBufferRTV         = nullptr;
    ID3D11Buffer* VertexBuffer                     = nullptr;

    // 4
    unsigned int NumVerticesSphere                 = 0;
    unsigned int Stride                            = 0;
};

class UBall
{
public:
    UBall() {}
    ~UBall()
    {
        Release();
    }

private:
    UBall(const UBall&);
    UBall& operator=(const UBall&);

public:
    void Initialize(const URenderer& InRenderer);
    void Update(URenderer& Renderer);
    void Release();
    void Render(URenderer& Renderer);

public:
    // getter
    inline FVector3             GetWorldPosition()const                             { return WorldPosition; }
    inline FVector3             GetVelocity()const                                  { return Velocity; }
    inline FVector3             GetNextVelocity()const                              { return NextVelocity; }
    inline float                GetRadius()const                                    { return Radius; }
    inline float                GetMass()const                                      { return Mass; }
    inline ID3D11Buffer*        GetConstBuffer()const                               { return ConstantBuffer; }
    
    //setter
    inline void                 SetWorldPosition(const FVector3& InNewPosition)     { WorldPosition = InNewPosition; }
    inline void                 SetVelocity(const FVector3& newVelocity)            { Velocity = newVelocity; }
    inline void                 SetNextVelocity(const FVector3& newNextVelocity)    { NextVelocity = newNextVelocity; }
    inline void                 SetRadius(float newRadius)                          { Radius = newRadius; }
    inline void                 SetMass(float newMass)                              { Mass = newMass; }
    inline void                 SetConstBuffer(ID3D11Buffer* const newConstBuffer)  { ConstantBuffer = newConstBuffer; }


   
private:

    void CreateConstantBuffer(const URenderer& InRenderer)
    {
        D3D11_BUFFER_DESC constantbufferdesc = {};
        constantbufferdesc.ByteWidth = (sizeof(FConstants) + 0xf) & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
        constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
        constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        ID3D11Device* Device = InRenderer.GetDevice();
        if (!Device)
        {
            OutputDebugStringA("Device is nullptr\n");
        }
        Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
    }
private:
    FVector3 WorldPosition{};  
    FVector3 Velocity{};
    FVector3 NextVelocity{};
    ID3D11Buffer* ConstantBuffer{};
    float Radius {};
    float Mass {};
};

class UBallNode
{
public:
    UBallNode() {}
    ~UBallNode() { Release(); }

private:
    UBallNode(const UBallNode&);
    UBallNode& operator=(const UBallNode&);

public:
    void Initialize();
    //void Update(URenderer& Renderer);
    void Release();


    // getter
    inline UBall*           GetUBall()const                          { return Ball; }
    inline UBallNode*       GetNextNode()const                       { return NextNode; }
    inline UBallNode*       GetPrevNode()const                       { return PrevNode; }

    // setter
    inline void             SetNextNode(UBallNode* const InNextNode) { NextNode = InNextNode; }
    inline void             SetPrevNode(UBallNode* const InPrevNode) { PrevNode = InPrevNode; }
    inline void             SetUBall(UBall* const InBall)            { Ball = InBall; }
private:
    UBall*     Ball      = nullptr;
    UBallNode* NextNode  = nullptr;
    UBallNode* PrevNode  = nullptr;
};

class UBallList
{
public:
    UBallList() 
    {

    };
    ~UBallList() 
    {
        Release();
    };

private:
    UBallList(const UBallList&);
    UBallList& operator=(const UBallList&);


public:
    void Initialize();
    void UpdateAll(URenderer& Renderer);
    void MoveAll();
    void DoRenderAll(URenderer& Renderer);
    void Release();


    void Add(const URenderer& InRenderer);
    void Delete(URenderer& Renderer);
  
    // getter
    inline int          GetBallCount() const     { return BallCount; }
    inline UBallNode*   GetHead() const          { return HeadNode; }
    inline UBallNode*   GetTail() const          { return TailNode; }

private:
    UBallNode* HeadNode = nullptr;
    UBallNode* TailNode = nullptr;

    int BallCount = 0;
};






// 패키징 유연성, 캡슐화 증가, 기능확장성, 컴파일 의존도 약화 용 namespace 

namespace RandomUtil
{
    inline float CreateRandomFloat(const float fMin, const float fMax) { return fMin + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (fMax - fMin))); }
}


namespace CalculateUtil
{
    inline const FVector3 operator* (const FVector3& lhs, float rhs)            { return FVector3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs); }
    inline const FVector3 operator/ (const FVector3& lhs, float rhs)            { return FVector3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs); }
    inline const FVector3 operator+ (const FVector3& lhs, const FVector3& rhs)  { return FVector3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z); }
    inline const FVector3 operator- (const FVector3& lhs, const FVector3& rhs)  { return FVector3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }

    inline const float Length(const FVector3& lhs)                              { return sqrtf(lhs.x * lhs.x + lhs.y * lhs.y + lhs.z * lhs.z); }
    inline const float Dot(const FVector3& lhs, const FVector3& rhs)            { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }
}


namespace CollisionUtil
{
    void BallMove(UBallList* BallList);
    void CollisionBallMove(UBallList* BallList);
    void GravityBallMove(UBallList* BallList);
    void ResolveCollision(UBall* pStartBall, UBall* pNextBall);

    void PerfectElasticCollision(UBallList& BallList);
}



extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 각종 메시지를 처리할 함수
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    switch (message)
    {
    case WM_DESTROY:
        // Signal that the app should quit
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}





//변수 설정
const float GGravityValue = 0.00098f; // 프레임당 중력가속도 (값은 튜닝 가능)

const float GLeftBorder = -1.0f;
const float GRightBorder = 1.0f;
const float GTopBorder = 1.0f;
const float GBottomBorder = -1.0f;

const float GElastic = 0.7f; // 탄성계수

int  TargetBallCount = 1;
bool bWasIsGravity = true;
bool bIsGravity = false;
bool bIsExit = false;




int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif


    //윈도우 창 설정
    WCHAR WindowClass[] = L"JungleWindowClass";
    WCHAR Title[] = L"Game Tech Lab";
    WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
    RegisterClassW(&wndclass);
    HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
        nullptr, nullptr, hInstance, nullptr);

    // 파이프라인에 필요한 리소스들을 초기화하고 이후 상수버퍼만 update하는 방식으로 랜더링

    // 렌더러 초기 설정
    Renderer renderer;
    
    // Device, Devicecontext, Swapchain, Viewport를 설정한다
    // GPU와 통신할 수 있는 DC와 화면 송출을 위한 Swapchain, Viewport를 설정한다.
    renderer.Initialize(hWnd);
    
    
    // 파이프라인 리소스를 설정하고 파이프라인에 박는다
    // IA, VS, RS, PS, OM 등을 설정한다 하지만 constant buffer는 설정하지않는다. 이 후 이것을 따로 설정하여 업데이트하여 렌더링한다.
   // renderer.InitializeAndSetPipeline();

    //버텍스버퍼 생성
    //버텍스 버퍼는 공유되고 constant버퍼를 사용하여 업테이트에 사용한다. 따라서 렌더러에 저장된다.
    INT NumVerticesArrow = sizeof(arrowVertices) / sizeof(FVertexSimple);
    ID3D11Buffer* arrowVertexBuffer = renderer.CreateVertexBuffer(arrowVertices, sizeof(arrowVertices));
    //renderer.SetNumVerticesSphere(NumVerticesSphere);
    
    //renderer.CreateVertexBuffer(arrowVertices, sizeof(arrowVertices));

    //ImGui 생성
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.GetDevice(), renderer.GetDeviceContext());
    
    //FPS 제한을 위한 설정
    const int targetFPS = 60;
    const double targetFrameTime = 1000.0 / targetFPS; // 한 프레임의 목표 시간 (밀리초 단위)
    
    //고성능 타이머 초기화
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER startTime, endTime;
    double elapsedTime = 0.0;
    
    // 난수 시드 설정 (time.h 없이)
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    srand(static_cast<unsigned int>(counter.QuadPart));


    // 첫 객체 생성
    //UBallList BallList;
    //BallList.Initialize();
    //BallList.Add(renderer);

    float rotationDeg = 0.0f;
    float rotationDelta = 5.0f;
    //메인루프
    while (bIsExit == false)
    {
        QueryPerformanceCounter(&startTime);
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                bIsExit = true;
                break;
            }
            else if (msg.message == WM_KEYDOWN)
            {
                if (msg.wParam == VK_LEFT)
                {
                    // 좌회전
                    //RotateVertices(arrowVertices, numVerticesArrow, +step, px, py); // 반시계
                    rotationDeg -= rotationDelta;
                }
                else if (msg.wParam == VK_RIGHT)
                {
                    // 우회전
                    rotationDeg += rotationDelta;
                }
            }
        }
        // 입력볼 , 현재볼 개수 차이따라 삭제 or 생성

        



        // 공만 그리는 것이 아니라 imgui도 그린다
        // imgui도 그래픽 파이프라인을 쓰기 때문에 반드시 공을 그릴때 다시 설정
        // 해줘야한다
        renderer.Prepare();
        renderer.PrepareShader();



        renderer.UpdateConstant({ 0.0f, -0.9f, 0.0f }, 0.4f, rotationDeg);
        renderer.Render(arrowVertexBuffer, NumVerticesArrow);


        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        ImGui::Begin("Jungle Property Window");
        ImGui::Text("Hello Jungle World!");
        ImGui::Checkbox("Gravity", &bIsGravity);
        ImGui::InputInt("Number of Balls", &TargetBallCount, 1, 5);
        if (ImGui::Button("Quit this app"))
        {
            // 현재 윈도우에 Quit 메시지를 메시지 큐로 보냄
            PostMessage(hWnd, WM_QUIT, 0, 0);
        }
        ImGui::End();


        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // 다 그렸으면 버퍼를 교환
        renderer.SwapBuffer();

        do
        {
            Sleep(0);

            // 루프 종료 시간 기록
            QueryPerformanceCounter(&endTime);

            // 한 프레임이 소요된 시간 계산 (밀리초 단위로 변환)
            elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

        } while (elapsedTime < targetFrameTime);
        ////////////////////////////////////////////
    }

    // ImGui 소멸
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    

 
    return 0;
}



// Ball 함수 정의
//void UBall::Initialize(const URenderer& InRenderer)
//{
//    Radius = RandomUtil::CreateRandomFloat(0.05f, 0.2f);
//    WorldPosition = FVector3(RandomUtil::CreateRandomFloat(-1.0f + Radius, 1.0f - Radius), RandomUtil::CreateRandomFloat(-1.0f + Radius, 1.0f - Radius), 0.0f);
//    Velocity = FVector3(RandomUtil::CreateRandomFloat(-0.01f, 0.01f), RandomUtil::CreateRandomFloat(-0.01f, 0.01f), 0.0f);
//    Mass = Radius * Radius;
//    CreateConstantBuffer(InRenderer);
//}
//
//void UBall::Update(URenderer& Renderer)
//{
//    Renderer.UpdateConstant(WorldPosition, Radius, ConstantBuffer);
//}
//
//void UBall::Render(URenderer& Renderer)
//{
//    Renderer.SetConstantBuffer(ConstantBuffer);
//    Renderer.Render();
//
//    //ResetOffset();
//}   
//
//void UBall::Release()
//{
//    ConstantBuffer->Release();
//    ConstantBuffer = nullptr;
//}
//
//void UBallNode::Initialize()
//{
//    
//}
//
//void UBallNode::Release()
//{
//    delete Ball;
//    //Ball->Release();
//}
//
//
//
//void UBallList::MoveAll()
//{
//    CollisionUtil::PerfectElasticCollision(*this);
//}
//
//
//void UBallList::DoRenderAll(URenderer& InRenderer)
//{
//    for (UBallNode* curr = HeadNode; curr; curr = curr->GetNextNode())
//    {
//        if (curr->GetUBall() == nullptr) continue;
//        curr->GetUBall()->Render(InRenderer);
//    }
//}
//
//
//void UBallList::Add(const URenderer& InRenderer) {
//    UBall* NewBall = new UBall;
//    NewBall->Initialize(InRenderer);
//
//    UBallNode* NewNode = new UBallNode;
//    NewNode->SetUBall(NewBall);
//
//    if (BallCount == 0) {
//        HeadNode->SetNextNode(NewNode);
//        NewNode->SetPrevNode(HeadNode);
//        NewNode->SetNextNode(TailNode);
//        TailNode->SetPrevNode(NewNode);
//    }
//    else {
//        TailNode->GetPrevNode()->SetNextNode(NewNode);
//        NewNode->SetPrevNode(TailNode->GetPrevNode());
//        NewNode->SetNextNode(TailNode);
//        TailNode->SetPrevNode(NewNode);
//    }
//    ++BallCount;
//}
//
//
//void UBallList::Delete(URenderer& Renderer)
//{
//    UBallNode* TargetNode = HeadNode->GetNextNode();
//
//    int TargetNumber = rand() % BallCount;
//
//    for (int i = 0; i < TargetNumber; ++i)
//    {
//        TargetNode = TargetNode->GetNextNode();
//    }
//
//    TargetNode->GetPrevNode()->SetNextNode(TargetNode->GetNextNode());
//    TargetNode->GetNextNode()->SetPrevNode(TargetNode->GetPrevNode());
//
//    delete TargetNode;
//
//    BallCount--;
//}
//
//
//void UBallList::Initialize()
//{
//    UBallNode* NewHeadNode = new UBallNode;
//    UBallNode* NewTailNode = new UBallNode;
//
//    HeadNode = NewHeadNode;
//    TailNode = NewTailNode;
//
//    HeadNode->SetNextNode(TailNode);
//    TailNode->SetPrevNode(HeadNode);
//}
//
//void UBallList::UpdateAll(URenderer& Renderer)
//{
//    for (UBallNode* curr = HeadNode; curr; curr = curr->GetNextNode())
//    {
//        if (curr->GetUBall() == nullptr) continue;
//        curr->GetUBall()->Update(Renderer);
//    }
//}
//
//
//void UBallList::Release()
//{
//    UBallNode* curr = HeadNode;
//    while (curr)
//    {
//        UBallNode* next = curr->GetNextNode();
//        delete curr;
//        curr = next;
//    }
//    HeadNode = TailNode = nullptr;
//}
//
//
//void CollisionUtil::BallMove(UBallList* BallList)
//{
//    auto iterStartNode = BallList->GetHead();
//
//    if (!iterStartNode)return;
//    
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // pStartBall 이 valid한지 체크
//        if (!pNowBall)continue;
//        
//        // 현재 공의 위치와 속도를 가져옴
//        FVector3 BallVelocity = pNowBall->GetVelocity();
//        FVector3 Offset = BallVelocity;
//
//        // 크기 가져오기
//        float Ballradius = pNowBall->GetRadius();
//
//        
//        
//              
//      
//
//        // 다음 위치가 경계접촉 시 처리 
//        if (Offset.x < GLeftBorder + Ballradius)
//        {
//            BallVelocity.x *= -1.0f;
//        }
//        if (Offset.x > GRightBorder - Ballradius)
//        {
//            BallVelocity.x *= -1.0f;
//        }
//        if (Offset.y < GTopBorder + Ballradius)
//        {
//            BallVelocity.y *= -1.0f;
//        }
//        if (Offset.y > GBottomBorder - Ballradius)
//        {
//            BallVelocity.y *= -1.0f;
//        }
//        //pNowBall->SetLocation(NextBallLocation);
//        pNowBall->SetVelocity(BallVelocity);
//    } 
//}
//
//void CollisionUtil::CollisionBallMove(UBallList* BallList)
//{
//    auto iterNowNode = BallList->GetHead();
//
//    if (!iterNowNode) return;
//    
//    for (; iterNowNode != nullptr; iterNowNode = iterNowNode->GetNextNode())
//    {
//        UBall* pBallA = iterNowNode->GetUBall();
//
//        auto iterNextNode = iterNowNode->GetNextNode();
//        if (!iterNextNode) continue;
//
//
//        for (; iterNextNode != nullptr; iterNextNode = iterNextNode->GetNextNode())
//        {
//            UBall* pBallB = iterNextNode->GetUBall();
//
//            // 두개의 공이 valid한지 체크
//            if (pBallA && pBallB)
//            {
//                float dist = CalculateUtil::Length(CalculateUtil::operator-(pBallA->GetWorldPosition(), pBallB->GetWorldPosition()));
//                if (dist <= pBallA->GetRadius() + pBallB->GetRadius())
//                {
//                    CollisionUtil::ResolveCollision(pBallA, pBallB);
//                }
//            }
//        }   
//    }   
//}
//
//void ResolveGravityCollision(UBall* pStartBall, UBall* pAnotherBall)
//{
//    FVector3 posA = pStartBall->GetWorldPosition();
//    FVector3 posB = pAnotherBall->GetWorldPosition();
//
//    FVector3 velA = pStartBall->GetVelocity();
//    FVector3 velB = pAnotherBall->GetVelocity();
//
//    float massA = pStartBall->GetMass();
//    float massB = pAnotherBall->GetMass();
//
//    // 두 공의 위치차이를 계산
//    FVector3 delta(posA.x - posB.x, posA.y - posB.y, 0);
//    float distSquared = delta.x * delta.x + delta.y * delta.y;
//    if (distSquared == 0.0f) return;
//
//    float distance = sqrtf(distSquared);
//    FVector3 normal(delta.x / distance, delta.y / distance, 0); // normalize = n
//
//    // 공끼리 충돌시에서 위치보정
//    float overlap = pStartBall->GetRadius() + pAnotherBall->GetRadius() - distance;
//    if (overlap > 0.0f)
//    {
//       FVector3 correction = CalculateUtil::operator*(normal, (overlap / 2.0f));
//       posA = CalculateUtil::operator+(posA, correction);
//       posB = CalculateUtil::operator-(posB, correction);
//       pStartBall->SetWorldPosition(posA);
//       pAnotherBall->SetWorldPosition(posB);
//    }
//
//    FVector3 relVel(velA.x - velB.x, velA.y - velB.y, 0); // 상대 속도
//
//    // 상대 속도의 충돌 방향 성분
//    float relSpeed = CalculateUtil::Dot(relVel, normal);
//
//    // 너무 작으면 무시
//    const float MinimumRelativeSpeed = 0.001f;
//    if (relSpeed >= 0.0f || fabs(relSpeed) < MinimumRelativeSpeed) return;
//        
//    // 탄성 충돌 반응 
//    float impulse = (-(1 + GElastic) * relSpeed) / ((1 / massA) + (1 / massB)); // = j
//
//    FVector3 impulseA(normal.x * (impulse / massA), normal.y * (impulse / massA), 0);
//    FVector3 impulseB(normal.x * (impulse / massB), normal.y * (impulse / massB), 0);
//
//    FVector3 NewVelocityA(velA.x + impulseA.x, velA.y + impulseA.y, 0);
//    FVector3 NewVelocityB(velB.x - impulseB.x, velB.y - impulseB.y, 0);
//
//    if (CalculateUtil::Length(NewVelocityA) < 0.004f) NewVelocityA = FVector3(0, 0, 0);
//    if (CalculateUtil::Length(NewVelocityB) < 0.004f) NewVelocityB = FVector3(0, 0, 0);
//
//
//    pStartBall->SetNextVelocity(NewVelocityA);
//    pAnotherBall->SetNextVelocity(NewVelocityB);
//}
//
//void CollisionUtil::GravityBallMove(UBallList*  BallList)
//{
//    if (!BallList) return;
//
//    auto iterStartNode = BallList->GetHead();
//
//    if (!iterStartNode) return;
//
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // pStartBall 이 valid한지 체크
//        if (!pNowBall)continue;
//
//        pNowBall->SetNextVelocity(pNowBall->GetVelocity());
//    }
//
//    iterStartNode = BallList->GetHead();
//
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();  
//
//        // pStartBall 이 valid한지 체크
//        if (!pNowBall)continue;
//
//        // 크기 가져오기
//        const float Ballradius = pNowBall->GetRadius();
//
//        auto iterNextNode = iterStartNode->GetNextNode();
//        if (!iterNextNode) continue;
//
//
//        // 이과정을 통해 충돌 반응으로 nextgravityvelocity가 누적으로 더해짐
//        for (; iterNextNode != nullptr; iterNextNode = iterNextNode->GetNextNode())
//        {
//            UBall* pAnotherBall = iterNextNode->GetUBall();
//
//            // 두개의 공이 valid한지 체크
//            if ((!pNowBall || !pAnotherBall))continue;
//            
//            
//            
//            float dist = CalculateUtil::Length(CalculateUtil::operator-(pNowBall->GetWorldPosition(), pAnotherBall->GetWorldPosition()));
//            if (dist <= pNowBall->GetRadius() + pAnotherBall->GetRadius())
//            {
//                ResolveGravityCollision(pNowBall, pAnotherBall);           
//            }
//        }            
//    }
//
//    // 한번에 위치값 조정
//    iterStartNode = BallList->GetHead();
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // 크기 가져오기
//        float Ballradius = pNowBall->GetRadius();
//
//        // pStartBall 이 valid한지 체크
//        if (!pNowBall)continue;
//
//        // 벽면 충돌 속도 수정 전 위치
//        FVector3 NextBallLocation = CalculateUtil::operator+(CalculateUtil::operator+(pNowBall->GetWorldPosition(), pNowBall->GetNextVelocity()), FVector3(0, -GGravityValue * 0.5f, 0));
//        pNowBall->SetNextVelocity({ pNowBall->GetNextVelocity().x, pNowBall->GetNextVelocity().y - GGravityValue, 0 });
//
//        // 경계접촉
//        if (NextBallLocation.x < GLeftBorder + Ballradius)
//        {
//            NextBallLocation.x = GLeftBorder + Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.x *= -GElastic;
//
//            if (fabs(Tempvelocity.x) < 0.005f) Tempvelocity.x = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//
//        }
//        if (NextBallLocation.x > GRightBorder - Ballradius)
//        {
//            NextBallLocation.x = GRightBorder - Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.x *= -GElastic;
//
//            if (fabs(Tempvelocity.x) < 0.005f) Tempvelocity.x = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        if (NextBallLocation.y < GTopBorder + Ballradius)
//        {
//            NextBallLocation.y = GTopBorder + Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.y *= -GElastic;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        if (NextBallLocation.y > GBottomBorder - Ballradius)
//        {
//            NextBallLocation.y = GBottomBorder - Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.y *= -GElastic;
//
//            if (fabs(Tempvelocity.y) < 0.05f) Tempvelocity.y = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        pNowBall->SetWorldPosition(NextBallLocation);
//        pNowBall->SetVelocity(pNowBall->GetNextVelocity());
//    }
//}
//
//void CollisionUtil::ResolveCollision(UBall* OutBallA, UBall* OutBallB)
//{
//    FVector3 posA = OutBallA->GetWorldPosition();
//    FVector3 posB = OutBallB->GetWorldPosition();
//
//    FVector3 velA = OutBallA->GetVelocity();
//    FVector3 velB = OutBallB->GetVelocity();
//
//    float massA = OutBallA->GetMass();
//    float massB = OutBallB->GetMass();
//
//    // ||p1 - p2||
//    float distance = CalculateUtil::Length(CalculateUtil::operator-(posA, posB));
//
//    // 노말단위벡터
//    FVector3 normalizedvector = CalculateUtil::operator-(posA, posB);
//    normalizedvector.x /= distance;
//    normalizedvector.y /= distance;
//
//
//
//    FVector3 relativeVel = CalculateUtil::operator-(velA, velB);
//
//
//    float velAlongNormal = CalculateUtil::Dot(relativeVel, normalizedvector);
//
//    // 충돌은 법선 방향으로만 일어난다
//    // 따라서 상대속도와 노말벡터와 정사영한다
//    if (velAlongNormal > 0.0f || fabs(velAlongNormal) < 0.0001f)
//    {
//        return;
//    }
//
//    float invMassA = 1.0f / massA;
//    float invMassB = 1.0f / massB;
//
//    float j = 0.0f;
//    if (bIsGravity)
//    {
//        j = -(1 + GElastic) * velAlongNormal / (invMassA + invMassB);
//    }
//    else
//    {
//        j = -(2) * velAlongNormal / (invMassA + invMassB);
//    }
//
//    FVector3 impulse = CalculateUtil::operator*(normalizedvector, j);
//
//    FVector3 newVelA = CalculateUtil::operator+(velA, CalculateUtil::operator*(impulse, invMassA));
//    FVector3 newVelB = CalculateUtil::operator-(velB, CalculateUtil::operator*(impulse, invMassB));
//
//    OutBallA->SetVelocity(newVelA);
//    OutBallB->SetVelocity(newVelB);
//
//    // 침투 보정
//   float penetration = OutBallA->GetRadius() + OutBallB->GetRadius() - distance;
//   if (penetration > 0.0f)
//   {
//       float correctionRatio = 0.5f;
//       FVector3 correction = CalculateUtil::operator*(normalizedvector, penetration * correctionRatio);
//   
//       OutBallA->SetWorldPosition(CalculateUtil::operator+(posA, correction));
//       OutBallB->SetWorldPosition(CalculateUtil::operator-(posB, correction));
//   }
//
//
//}
//
//void CollisionUtil::PerfectElasticCollision(UBallList& BallList)
//{
//    auto iterNowNode = BallList.GetHead()->GetNextNode();
//
//    for (; iterNowNode != nullptr; iterNowNode = iterNowNode->GetNextNode())
//    {
//        auto iterNextNode = iterNowNode->GetNextNode();
//    
//        for (; iterNextNode != nullptr; iterNextNode = iterNextNode->GetNextNode())
//        {
//            UBall* pBallA = iterNowNode->GetUBall();
//            UBall* pBallB = iterNextNode->GetUBall();
//    
//            // 두개의 공이 valid한지 체크
//            if (pBallA == nullptr || pBallB == nullptr)
//            {
//                continue;
//            }
//            float dist = CalculateUtil::Length(CalculateUtil::operator-(pBallA->GetWorldPosition(), pBallB->GetWorldPosition()));
//            if (dist <= pBallA->GetRadius() + pBallB->GetRadius())
//            {
//                CollisionUtil::ResolveCollision(pBallA, pBallB);
//            }
//    
//        }
//    }
//
//    iterNowNode = BallList.GetHead()->GetNextNode();
//
//    while (iterNowNode != BallList.GetTail())
//    {
//        UBall* pBallA = iterNowNode->GetUBall();
//
//        // 현재 공의 위치와 속도를 가져옴
//        FVector3 BallVelocity = pBallA->GetVelocity();
//        FVector3 Position = pBallA->GetWorldPosition();
//
//        // 중력 가속도 적용
//        if (bIsGravity)
//        {
//            BallVelocity.y -= GGravityValue;
//        }
//        
//
//
//
//        FVector3 NewPosition = CalculateUtil::operator+(Position, BallVelocity);
//
//
//        // 크기 가져오기
//        float Ballradius = pBallA->GetRadius();
//
//
//        // 다음 위치가 경계접촉 시 처리 
//        if (NewPosition.x < GLeftBorder + Ballradius)
//        {
//            NewPosition.x = GLeftBorder + Ballradius;
//            if (bIsGravity)
//            {
//                BallVelocity.x *= -GElastic;
//            }
//            else
//            {
//                BallVelocity.x *= -1.0f;
//            }
//        }
//
//        if (NewPosition.x > GRightBorder - Ballradius)
//        {
//            NewPosition.x = GRightBorder - Ballradius;
//            if (bIsGravity)
//            {
//                BallVelocity.x *= -GElastic;
//            }
//            else
//            {
//                BallVelocity.x *= -1.0f;
//            }
//        }
//
//        if (NewPosition.y > GTopBorder - Ballradius)
//        {
//            NewPosition.y = GTopBorder - Ballradius;
//            if (bIsGravity)
//            {
//                BallVelocity.y *= -GElastic;
//            }
//            else
//            {
//                BallVelocity.y *= -1.0f;
//            }
//        }
//
//        if (NewPosition.y < GBottomBorder + Ballradius)
//        {
//            NewPosition.y = GBottomBorder + Ballradius;
//            if (bIsGravity)
//            {
//                BallVelocity.y *= -GElastic;
//            }
//            else
//            {
//                BallVelocity.y *= -1.0f;
//            }
//        }
//
//
//        //if (fabsf(BallVelocity.x) < 0.0005f)
//        //    BallVelocity.x = 0.0f;
//        //if (fabsf(BallVelocity.y) < 0.0005f)
//        //    BallVelocity.y = 0.0f;
//
//        pBallA->SetVelocity(BallVelocity);
//        pBallA->SetWorldPosition(NewPosition);
//
//        iterNowNode = iterNowNode->GetNextNode();
//    }
//}



//void CollisionUtil::GravityBallMove(UBallList* BallList)
//{
//    if (!BallList) return;
//
//    auto iterStartNode = BallList->GetHead();
//
//    if (!iterStartNode) return;
//
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // pStartBall 이 valid한지 체크
//        if (!pNowBall)continue;
//
//        pNowBall->SetNextVelocity(pNowBall->GetVelocity());
//    }
//
//    iterStartNode = BallList->GetHead();
//
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // pStartBall 이 valid한지 체크
//        if (!pNowBall)continue;
//
//        // 크기 가져오기
//        const float Ballradius = pNowBall->GetRadius();
//
//        auto iterNextNode = iterStartNode->GetNextNode();
//        if (!iterNextNode) continue;
//
//
//        // 이과정을 통해 충돌 반응으로 nextgravityvelocity가 누적으로 더해짐
//        for (; iterNextNode != nullptr; iterNextNode = iterNextNode->GetNextNode())
//        {
//            UBall* pAnotherBall = iterNextNode->GetUBall();
//
//            // 두개의 공이 valid한지 체크
//            if ((!pNowBall || !pAnotherBall))continue;
//
//
//
//            float dist = CalculateUtil::Length(CalculateUtil::operator-(pNowBall->GetLocation(), pAnotherBall->GetLocation()));
//            if (dist <= pNowBall->GetRadius() + pAnotherBall->GetRadius())
//            {
//                ResolveGravityCollision(pNowBall, pAnotherBall);
//            }
//        }
//    }
//
//    // 한번에 위치값 조정
//    iterStartNode = BallList->GetHead();
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // 크기 가져오기
//        float Ballradius = pNowBall->GetRadius();
//
//        // pStartBall 이 valid한지 체크
//        if (!pNowBall)continue;
//
//        // 벽면 충돌 속도 수정 전 위치
//        FVector3 NextBallLocation = CalculateUtil::operator+(CalculateUtil::operator+(pNowBall->GetLocation(), pNowBall->GetNextVelocity()), FVector3(0, -GGravityValue * 0.5f, 0));
//        pNowBall->SetNextVelocity({ pNowBall->GetNextVelocity().x, pNowBall->GetNextVelocity().y - GGravityValue, 0 });
//
//        // 경계접촉
//        if (NextBallLocation.x < GLeftBorder + Ballradius)
//        {
//            NextBallLocation.x = GLeftBorder + Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.x *= -GElastic;
//
//            if (fabs(Tempvelocity.x) < 0.005f) Tempvelocity.x = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//
//        }
//        if (NextBallLocation.x > GRightBorder - Ballradius)
//        {
//            NextBallLocation.x = GRightBorder - Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.x *= -GElastic;
//
//            if (fabs(Tempvelocity.x) < 0.005f) Tempvelocity.x = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        if (NextBallLocation.y < GTopBorder + Ballradius)
//        {
//            NextBallLocation.y = GTopBorder + Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.y *= -GElastic;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        if (NextBallLocation.y > GBottomBorder - Ballradius)
//        {
//            NextBallLocation.y = GBottomBorder - Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.y *= -GElastic;
//
//            if (fabs(Tempvelocity.y) < 0.05f) Tempvelocity.y = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        pNowBall->SetLocation(NextBallLocation);
//        pNowBall->SetVelocity(pNowBall->GetNextVelocity());
//    }
//}
