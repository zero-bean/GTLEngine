#include "TestScene.h"

TestScene::TestScene()
{
}

void TestScene::Start()
{
    BallList.Initialize();
    BallList.Add(renderer);
}

void TestScene::Update(float deltaTime)
{
    // 입력볼 , 현재볼 개수 차이따라 삭제 or 생성

    if (BallList.GetBallCount() < TargetBallCount)
    {
        int iAddBallCount = TargetBallCount - BallList.GetBallCount();
        for (int i = 0; i < iAddBallCount; ++i)
        {
            BallList.Add(renderer);
        }
    }
    else if (BallList.GetBallCount() > TargetBallCount)
    {
        int iDelBallCount = BallList.GetBallCount() - TargetBallCount;
        for (int i = 0; i < iDelBallCount; ++i)
        {
            BallList.Delete(renderer);
        }
    }
}

void TestScene::OnGUI(HWND hWND)
{
    ImGui::Text("Hello Jungle World!");
    ImGui::Checkbox("Gravity", &bIsGravity);
    ImGui::InputInt("Number of Balls", &TargetBallCount, 1, 5);
    if (ImGui::Button("Quit this app"))
    {
        // 현재 윈도우에 Quit 메시지를 메시지 큐로 보냄
        PostMessage(hWND, WM_QUIT, 0, 0);
    }
}

void TestScene::OnRender(URenderer* renderer)
{
    BallList.MoveAll();
    BallList.UpdateAll(renderer);
    BallList.DoRenderAll(renderer);
}

void TestScene::Shutdown()
{
}
