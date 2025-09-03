#include "TestScene.h"
#include "ArrowVertices.h"
#include "ScreenUtil.h"

void TestScene::Start()
{
    playerarrow.Initialize(*renderer);

    NumVerticesArrow = sizeof(arrowVertices) / sizeof(FVertexSimple);
    arrowVertexBuffer = renderer->CreateVertexBuffer(arrowVertices, sizeof(arrowVertices));


    ShotBall = nullptr;

    for (int i = 0;i < ROWS; ++i)
    {
        for (int j = 0;j < COLS; ++j)
        {
           balls[i][j] = nullptr;  
        }
    }
}

void TestScene::Update(float deltaTime)
{
    // 입력볼 , 현재볼 개수 차이따라 삭제 or 생성
    playerarrow.Update(*renderer);

    if (ShotBall != nullptr)
    {
        ShotBall->Update(*renderer);
    }
}

void TestScene::LateUpdate(float deltaTime)
{
    if (ShotBall == nullptr)
    {
        return;
    }
    FVector3 ShotBallPosition = ShotBall->GetWorldPosition();
    FVector3 ShotBallVelocity = ShotBall->GetVelocity();

    //천장접촉
    if (ShotBallPosition.y + 0.11f >= 1)
    {
        int dx = std::round((ShotBallPosition.x + 1) / 2.0f * static_cast<float>(COLS - 1));
        balls[0][dx] = ShotBall;

        balls[0][dx]->SetVelocity({ 0,0,0 });
        balls[0][dx]->SetWorldPosition({ 2.0f * static_cast<float>(dx) / 8.0f - 1.0f, 0.89f, 0.0f });
        balls[0][dx]->SetBallType(eBallType::Static);
        ShotBall = nullptr;
    }

    //왼쪽 벽이랑 닿았을때
    if (ShotBall && (ShotBallPosition.x - 0.11f <= -1.0f * ScreenUtil::GetAspectRatio()))
    {
        ShotBall->SetWorldPosition({ -ScreenUtil::GetAspectRatio() + 0.11f, ShotBallPosition.y, ShotBallPosition.z });
        ShotBall->SetVelocity({ -ShotBallVelocity.x, ShotBallVelocity.y, ShotBallVelocity.z });
    }

    //오른쪽 벽이랑 닿았을때
    if (ShotBall && (ShotBallPosition.x + 0.11f >= 1.0f * ScreenUtil::GetAspectRatio()))
    {
        ShotBall->SetWorldPosition({ ScreenUtil::GetAspectRatio() - 0.11f, ShotBallPosition.y, ShotBallPosition.z });
        ShotBall->SetVelocity({ -ShotBallVelocity.x, ShotBallVelocity.y, ShotBallVelocity.z });
    }



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
        else if (msg.wParam == VK_SPACE)
        {
            // 총알생성
            //playerarrow.SetDegree(rotationDelta);
            if (ShotBall == nullptr)
            {
                ShotBall = new Ball;
                ShotBall->Initialize(*renderer);
                ShotBall->SetRadius(0.11f);

                float speed = 0.02f; // 원하는 속도 값
                ShotBall->SetVelocity({ -cosf(DirectX::XMConvertToRadians(playerarrow.GetDegree() + 90)) * speed,
                                        sinf(DirectX::XMConvertToRadians(playerarrow.GetDegree() + 90)) * speed,
                                        0.0f });
              
            
                //ShotBall->SetVelocity( 
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
    playerarrow.Render(*renderer);

    //전체 배열 렌더
    for (int i = 0;i < ROWS; ++i)
    {
        for (int j = 0;j < COLS; ++j)
        {
            if (balls[i][j] != nullptr)
            {
                balls[i][j]->Render(*renderer);
            }
        }
    }

    //발사총알 렌더
    if (ShotBall != nullptr)
    {
        ShotBall->Render(*renderer);
    }
}

void TestScene::Shutdown()
{
}
