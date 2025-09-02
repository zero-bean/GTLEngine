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
using namespace DirectX;

#include "SceneManager.h"
#include "TestScene.h"
#include "Sphere.h"


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
    //INT NumVerticesArrow = sizeof(arrowVertices) / sizeof(FVertexSimple);
    //ID3D11Buffer* arrowVertexBuffer = renderer.CreateVertexBuffer(arrowVertices, sizeof(arrowVertices));
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

    SceneManager* sceneManager = SceneManager::GetInstance();
    TestScene* testScene = new TestScene(&renderer);
    sceneManager->SetScene(testScene);
    

    // 첫 객체 생성
    float rotationDeg = 0.0f;
    float rotationDelta = 5.0f;
    //메인루프
    while (bIsExit == false)
    {
        QueryPerformanceCounter(&startTime);

        sceneManager->Update(elapsedTime);
        



        // 공만 그리는 것이 아니라 imgui도 그린다
        // imgui도 그래픽 파이프라인을 쓰기 때문에 반드시 공을 그릴때 다시 설정
        // 해줘야한다
        renderer.Prepare();
        renderer.PrepareShader();
        sceneManager->OnRender();


        //renderer.UpdateConstant({ 0.0f, -0.9f, 0.0f }, 0.4f, rotationDeg);
        //renderer.Render(arrowVertexBuffer, NumVerticesArrow);


        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        ImGui::Begin("Jungle Property Window");
        sceneManager->OnGUI(hWnd);
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