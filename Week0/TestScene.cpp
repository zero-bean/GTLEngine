#include "TestScene.h"
#include "ArrowVertices.h"

void TestScene::Start()
{
    playerarrow.Initialize(*renderer);

    NumVerticesArrow = sizeof(arrowVertices) / sizeof(FVertexSimple);
    arrowVertexBuffer = renderer->CreateVertexBuffer(arrowVertices, sizeof(arrowVertices));
}

void TestScene::Update(float deltaTime)
{
    // 입력볼 , 현재볼 개수 차이따라 삭제 or 생성
    playerarrow.Update(*renderer);
}

void TestScene::LateUpdate(float deltaTime)
{
}

void TestScene::OnMessage(MSG msg)
{
    if (msg.message == WM_KEYDOWN)
    {
        if (msg.wParam == VK_LEFT)
        {
            // 좌회전
            playerarrow.SetDegree(-rotationDelta);
        }
        else if (msg.wParam == VK_RIGHT)
        {
            // 우회전
            playerarrow.SetDegree(rotationDelta);
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
    playerarrow.Render(*renderer);
}

void TestScene::Shutdown()
{
}
