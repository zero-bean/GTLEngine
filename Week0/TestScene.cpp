#include "TestScene.h"
#include "ArrowVertices.h"

void TestScene::Start()
{
    NumVerticesArrow = sizeof(arrowVertices) / sizeof(FVertexSimple);
    arrowVertexBuffer = renderer->CreateVertexBuffer(arrowVertices, sizeof(arrowVertices));
}

void TestScene::Update(float deltaTime)
{
    // 입력볼 , 현재볼 개수 차이따라 삭제 or 생성
    MSG msg;
    bool bIsExit = false;

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
}

void TestScene::OnGUI(HWND hWND)
{
    ImGui::Text("Hello Jungle World!");
    if (ImGui::Button("Quit this app"))
    {
        // 현재 윈도우에 Quit 메시지를 메시지 큐로 보냄
        PostMessage(hWND, WM_QUIT, 0, 0);
    }
}

void TestScene::OnRender()
{
    renderer->UpdateConstant({ 0.0f, -0.9f, 0.0f }, 0.4f, rotationDeg);
    renderer->Render(arrowVertexBuffer, NumVerticesArrow);
}

void TestScene::Shutdown()
{
}
