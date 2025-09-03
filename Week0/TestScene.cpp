#include "TestScene.h"
#include "ArrowVertices.h"
#include "ScreenUtil.h"

inline bool TestScene::IsInRange(const int x, const int y) const
{
    return (x >= 0 && x < ROWS && y >= 0 && y < COLS);
}

// 해당 위치의 볼과 4-방향 "인접한 같은 색상의 볼"을 탐색하고 반환합니다.
std::vector<std::pair<int, int>> TestScene::FindSameColorBalls(const std::pair<int, int>& start, const eBallColor& color)
{
    std::vector<std::pair<int, int>> sameColorBalls = {};
     const int dx[4] = { 1,-1, 0, 0 };
     const int dy[4] = { 0, 0, 1, -1 };
     const int sx = start.first;
     const int sy = start.second;
    
     // 방문 배열은 [R][C] = [y][x]
     std::vector<std::vector<bool>> visited(ROWS, std::vector<bool>(COLS, false));
     std::queue<std::pair<int, int>> q;
    
     //visited[sx][sy] = true;
     q.push({ sx, sy });
    
     // BFS
     while (!q.empty())
     {
         // 현재 좌표
         const int cx = q.front().first;
         const int cy = q.front().second;
         q.pop();
    
         if (visited[cx][cy] == true)
             continue;

         sameColorBalls.push_back({ cx,cy });
         visited[cx][cy] = true;

         // 4-방향 인접한 좌표 탐색
         for (int i = 0; i < 4; ++i)
         {
             const int nx = cx + dx[i];
             const int ny = cy + dy[i];
    
             // 유효한 범위가 아니라면
             if (IsInRange(nx, ny) == false) { continue; }
             // 이미 방문을 했다면
             if (visited[nx][ny] == true) { continue; }
             // 같은 색상이 아니라면
             if (board[nx][ny].ball == nullptr || board[nx][ny].ball->GetBallColor() != color) { continue; }
    
             q.push({ nx, ny });
         }

     }

     return sameColorBalls;
    
}


// 낙하할 볼을 탐색하고 반환합니다.
// 루트(맨 윗줄)과 연결되지 않은 공들을 찾아 반환합니다. (y,x) 쌍 목록
std::vector<std::pair<int, int>> TestScene::FindFloatingBalls()
{
    std::vector<std::pair<int, int>> floating = {};
    std::vector<std::vector<bool>> visited(ROWS, std::vector<bool>(COLS, false));
    std::queue<std::pair<int, int>> q = {};

    // 1) 맨 윗줄에서 공이 있는 칸들을 시작점으로 큐에 삽입
    for (int y = 0; y < COLS; ++y) {
        if (board[0][y].ball != nullptr && board[0][y].ball->GetBallState()==eBallState::Idle)
        {           // 공이 있는 칸만 시작
            visited[0][y] = true;
            q.push({ 0, y });               // (y,x)
        }
    }

    // 2) BFS: 공이 있는 칸들끼리만 연결을 탐색
    const int dx[4] = { 1, -1, 0, 0 };
    const int dy[4] = { 0, 0, 1, -1 };

    while (!q.empty()) {
        const int cx = q.front().first;
        const int cy = q.front().second;
        q.pop();

        for (int d = 0; d < 4; ++d) {
            const int nx = cx + dx[d];
            const int ny = cy + dy[d];

            if (!IsInRange(nx, ny)) { continue; }
            if (visited[nx][ny]) { continue; }
            if (board[nx][ny].ball == nullptr || board[nx][ny].ball->GetBallState() != eBallState::Idle) { continue; }

            visited[nx][ny] = 1;
            q.push({ nx, ny });
        }
    }

    // 3) 방문되지 않은 공(=루트 미연결)만 수집
    for (int x = 0; x < ROWS; ++x) {
        for (int y = 0; y < COLS; ++y) {
            if (board[x][y].ball != nullptr && !visited[x][y]) {
                floating.push_back({ x, y });
            }
        }
    }

    return floating;
}


void TestScene::Start()
{
    playerarrow.Initialize(*renderer);

    NumVerticesArrow = sizeof(arrowVertices) / sizeof(FVertexSimple);
    arrowVertexBuffer = renderer->CreateVertexBuffer(arrowVertices, sizeof(arrowVertices));

    ShotBall = nullptr;

  BallQueue = std::queue<Ball*>(); // 오류가 나는 줄
    //std::queue<Ball*> qTemp;
  for(int i = 0; i < 2; ++i)
  {
      Ball* ball = new Ball;
      ball->Initialize(*renderer);
      ball->SetRadius(0.11f);
  ball->SetWorldPosition({ 0.0f - (i * 0.22f), -0.9f, 0.0f});
      BallQueue.push(ball);
	}
  
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

                // 여기서 상태기반 if 문 들어가야함

            }
        }
    }


    for (int i = 0;i < ROWS; ++i)
    {
        for (int j = 0;j < COLS; ++j)
        {
            if (board[i][j].ball != nullptr && board[i][j].ball->GetBallState() != eBallState::Idle)
            {
                SAFE_DELETE(board[i][j].ball);
                board[i][j].ball = nullptr;
            }

            //여기서 모두 false처리
            board[i][j].bEnable = false;
        }
    }



    // 1차 true 처리
    for (int i = 0;i < ROWS; ++i)
    {
        for (int j = 0;j < COLS; ++j)
        {
            if (board[i][j].ball != nullptr) // 공이 있다면 
            {
                const int dx[4] = { 1, -1, 0, 0 };
                const int dy[4] = { 0, 0, 1, -1 };

                //유효성 검사
                for (int k = 0;k < 4;++k)
                {
                    const int nx = i + dx[k];
                    const int ny = j + dy[k];

                    if (!IsInRange(nx, ny)) { continue; }
                    if (board[nx][ny].ball != nullptr) { continue; }

                    board[nx][ny].bEnable = true;

                }
                
            }
        }
    }

    // 2차 벽면처리
    for (int i = 0;i < COLS; ++i)
    {
        if (board[0][i].ball == nullptr)
        {
            board[0][i].bEnable = true;
        }
    }


	BallQueue.back()->Update(*renderer);
	BallQueue.front()->Update(*renderer);


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
            
        /*
        1. 같은색상 찾기(dfs, bfs 아무거나) 자료구조(queue)에 넣기

        1-1 큐 사이즈가 일정 갯수 이하라면 그냥 통과

        1-2 큐 사이즈가 일정 갯수 이상이라면 파괴

        2. 1-2가 만족하면 추가 파괴 진행

        
        
        */

        // 1-2첫번째 파괴
         std::vector<std::pair<int, int>> ResultDieVector = FindSameColorBalls({ dx,dy }, board[dx][dy].ball->GetBallColor());
         if (ResultDieVector.size() >= 3)
        {
            for (auto& pos : ResultDieVector)
            {
                board[pos.first][pos.second].ball->SetBallState(eBallState::Die);
            }
            
        }
        // 2 두번째 파괴
        std::vector<std::pair<int, int>> ResultFallenVector = FindFloatingBalls();

        for (auto& pos : ResultFallenVector)
        {
            board[pos.first][pos.second].ball->SetBallState(eBallState::Fallen);
        }


        // 0: 다시하기, 1:게임오버, 2:게임클리어
        int bGameClear = 2;

        // 게임 오버 검사하기
        for (int i = 0;i < COLS;++i)
        {
            if (board[6][i].ball != nullptr)
            {
                //여기서 게임오버 실행
                bGameClear = 1;
            }
        }

        

        //게임 클리어 검사
        for (int i = 0;i < ROWS; ++i)
        {
            for (int j = 0;j < COLS; ++j)
            {
                if (board[i][j].ball != nullptr)
                {
                    bGameClear = 0;
                }
            }
        }



        

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
                /*ShotBall = new Ball;
                ShotBall->Initialize(*renderer);
                ShotBall->SetRadius(0.11f);*/

				ShotBall = BallQueue.front();
				BallQueue.pop();

                BallQueue.front()->SetWorldPosition({ 0.0f, -0.9f, 0.0f });

                Ball* ball = new Ball;
                ball->Initialize(*renderer);
                ball->SetRadius(0.11f);
                ball->SetWorldPosition({ 0.0f - 0.22f, -0.9f, 0.0f });
                BallQueue.push(ball);

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

    BallQueue.back()->Render(*renderer);
    BallQueue.front()->Render(*renderer);

    //발사총알 렌더
    if (ShotBall != nullptr)
    {
        ShotBall->Render(*renderer);
    }
}

void TestScene::Shutdown()
{
    while (!BallQueue.empty()) {
        Ball* b = BallQueue.front(); // 맨 앞 요소 가져오기
        BallQueue.pop();            // 맨 앞 요소 제거

        delete b;
    }
}
