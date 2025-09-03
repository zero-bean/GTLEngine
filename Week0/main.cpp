#include <windows.h>
#include <crtdbg.h>  

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "DirectXTK.lib")


#include <d3d11.h>
#include <d3dcompiler.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"


// �ڵ���Ģ 
//struct �� F�� ������




#include "pch.h"
using namespace DirectX;

#include "SceneManager.h"
#include "TestScene2.h"
#include "PlayerArrow.h"



extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ���� �޽����� ó���� �Լ�
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

//���� ����
const float GGravityValue = 0.00098f; // �����Ӵ� �߷°��ӵ� (���� Ʃ�� ����)

const float GLeftBorder = -1.0f;
const float GRightBorder = 1.0f;
const float GTopBorder = 1.0f;
const float GBottomBorder = -1.0f;

const float GElastic = 0.7f; // ź�����

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


    //������ â ����
    WCHAR WindowClass[] = L"JungleWindowClass";
    WCHAR Title[] = L"Game Tech Lab";
    WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
    RegisterClassW(&wndclass);
    HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
        nullptr, nullptr, hInstance, nullptr);


    // ������ �ʱ� ����
    Renderer renderer;
    
    renderer.Initialize(hWnd);
    



    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.GetDevice(), renderer.GetDeviceContext());
    
    const int targetFPS = 60;
    const double targetFrameTime = 1000.0 / targetFPS;
    
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER startTime, endTime;
    double elapsedTime = 0.0;

    SceneManager* sceneManager = SceneManager::GetInstance();
    TestScene2* testScene = new TestScene2(&renderer);
    sceneManager->SetScene(testScene);
    

    while (bIsExit == false)
    {
        QueryPerformanceCounter(&startTime);

        sceneManager->Update(elapsedTime);

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
            else
            {
                sceneManager->OnMessage(msg);
            }
        }

        sceneManager->LateUpdate(elapsedTime);

        renderer.Prepare();
        renderer.PrepareShader();
        sceneManager->OnRender();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        ImGui::Begin("Jungle Property Window");
        sceneManager->OnGUI(hWnd);
        ImGui::End();
        ImGui::Render();


        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        renderer.SwapBuffer();

        do
        {
            Sleep(0);

            QueryPerformanceCounter(&endTime);

            elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

        } while (elapsedTime < targetFrameTime);
        ////////////////////////////////////////////
    }

    // ImGui �Ҹ�
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    return 0;
}

