#include "TestScene.h"
#include "ArrowVertices.h"
#include "ScreenUtil.h"

inline bool TestScene::IsInRange(const int x, const int y) const
{
    return (x >= 0 && x < ROWS && y >= 0 && y < COLS);
}

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
          
           board[i][j].ball = nullptr;
           board[i][j].bEnable = false;
       }
   }


   // 임시 레벨(0,4)
   Ball* temp= new Ball;
   board[0][4].ball = temp;
   board[0][4].ball->Initialize(*renderer);
   board[0][4].ball->SetWorldPosition({ 0.0f, 0.89f, 0.0f });


    for (int i = 0; i < COLS; ++i)
        board[0][i].bEnable = true;
    board[0][4].bEnable = false;
    board[1][4].bEnable = true;

     
}

void TestScene::Update(float deltaTime)
{
    // 입력볼 , 현재볼 개수 차이따라 삭제 or 생성
    playerarrow.Update(*renderer);

    for (int i = 0;i < ROWS; ++i)
    {
        for (int j = 0;j < COLS; ++j)
        {
            if (board[i][j].ball != nullptr)
            {
                board[i][j].ball->Update(*renderer);
            }
        }
    }



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
    int dy = std::round((ShotBallPosition.x + 1) / 2.0f * static_cast<float>(COLS - 1)); // 이차원 배열의 가로 (첫 번째)
    int dx = ROWS - 2 - std::round((ShotBallPosition.y + 1) / 2.0f * static_cast<float>(ROWS - 1)); // 이차원 배열의 세로 (두 번째)
    FVector3 NewVector = { (dy - 4) * 0.22F, (dx - 4) * - 0.22f, 0.0f };

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

    if(board[dx][dy].bEnable == true)
    { 
        ShotBall->SetVelocity({ 0,0,0 });
        ShotBall->SetWorldPosition(NewVector);

        board[dx][dy].ball = ShotBall;
        board[dx][dy].bEnable = false;
        ShotBall = nullptr;


        const int cx[4] = { 1,-1,0,0 };
        const int cy[4] = { 0,0,1,-1 };

        for (int i = 0; i < 4; ++i)
        {
            // 상 하 좌 우 4군데 검사
            const int nx = dx + cx[i];
            const int ny = dy + cy[i];
            
            if (IsInRange(nx, ny))
            {
                if (board[nx][ny].bEnable == false && board[nx][ny].ball == nullptr)
                    board[nx][ny].bEnable = true;
            }
        }

        // 버블 삭제 함수 추가할 것
    
        // 게임 오버 검사하기
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
            if (board[i][j].ball !=  nullptr)
            {
                board[i][j].ball->Render(*renderer);
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
