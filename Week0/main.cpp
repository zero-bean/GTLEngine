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

#include "pch.h"



#include "PlayerArrow.h"



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

const float RotationDelta = 5.0f;



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


    // 렌더러 초기 설정
    Renderer renderer;
    
    renderer.Initialize(hWnd);



    PlayerArrow playerarrow;

    playerarrow.Initialize(renderer);

    ///////////////////////////////////////////////////////////////////
    //INT NumVerticesArrow = sizeof(ArrowVertices) / sizeof(FVertexSimple);
    //ID3D11Buffer* arrowVertexBuffer = renderer.CreateVertexBuffer(ArrowVertices, sizeof(ArrowVertices));
    //renderer.SetNumVerticesSphere(NumVerticesSphere);
    
    //renderer.CreateVertexBuffer(arrowVertices, sizeof(arrowVertices));
    /////////////////////////////////////////////////////////////////////////
    



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

    //float rotationDeg = 0.0f;
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
                    playerarrow.SetDegree(-rotationDelta);
                    //rotationDeg -= rotationDelta;


                }
                else if (msg.wParam == VK_RIGHT)
                {
                    // 우회전
                    playerarrow.SetDegree(rotationDelta);
                    //rotationDeg += rotationDelta;
                }
                else if (msg.wParam == VK_SPACE)
                {

                }
                
            }
        }

        renderer.Prepare();
        renderer.PrepareShader();

        playerarrow.Update(renderer);

        //renderer.UpdateConstant({ 0.0f, -0.9f, 0.0f }, 0.4f, rotationDeg);
        //renderer.Render(arrowVertexBuffer, NumVerticesArrow);

        playerarrow.Render(renderer);

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
