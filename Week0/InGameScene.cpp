#include "InGameScene.h"
#include "ArrowVertices.h"
#include "ScreenUtil.h"
#include "GameOverScene.h"
#include "ClearScene.h"
#include "TimeManager.h"

inline bool InGameScene::IsInRange(const int x, const int y) const
{
    return (x >= 0 && x < ROWS && y >= 0 && y < COLS);
}

// ?´ë‹¹ ?„ì¹˜??ë³¼ê³¼ 4-ë°©í–¥ "?¸ì ‘??ê°™ì? ?‰ìƒ??ë³????ìƒ‰?˜ê³  ë°˜í™˜?©ë‹ˆ??
std::vector<std::pair<int, int>> InGameScene::FindSameColorBalls(const std::pair<int, int>& start, const eBallColor& color)
{
    std::vector<std::pair<int, int>> sameColorBalls = {};
     const int dx[4] = { 1,-1, 0, 0 };
     const int dy[4] = { 0, 0, 1, -1 };
     const int sx = start.first;
     const int sy = start.second;
    
     // ë°©ë¬¸ ë°°ì—´?€ [R][C] = [y][x]
     std::vector<std::vector<bool>> visited(ROWS, std::vector<bool>(COLS, false));
     std::queue<std::pair<int, int>> q;
    
     //visited[sx][sy] = true;
     q.push({ sx, sy });
    
     // BFS
     while (!q.empty())
     {
         // ?„ìž¬ ì¢Œí‘œ
         const int cx = q.front().first;
         const int cy = q.front().second;
         q.pop();
    
         if (visited[cx][cy] == true)
             continue;

         sameColorBalls.push_back({ cx,cy });
         visited[cx][cy] = true;

         // 4-ë°©í–¥ ?¸ì ‘??ì¢Œí‘œ ?ìƒ‰
         for (int i = 0; i < 4; ++i)
         {
             const int nx = cx + dx[i];
             const int ny = cy + dy[i];
    
             // ? íš¨??ë²”ìœ„ê°€ ?„ë‹ˆ?¼ë©´
             if (IsInRange(nx, ny) == false) { continue; }
             // ?´ë? ë°©ë¬¸???ˆë‹¤ë©?
             if (visited[nx][ny] == true) { continue; }
             // ê°™ì? ?‰ìƒ???„ë‹ˆ?¼ë©´
             if (board[nx][ny].ball == nullptr || board[nx][ny].ball->GetBallColor() != color) { continue; }
    
             q.push({ nx, ny });
         }

     }

     return sameColorBalls;
    
}

// ?™í•˜??ë³¼ì„ ?ìƒ‰?˜ê³  ë°˜í™˜?©ë‹ˆ??
// ë£¨íŠ¸(ë§??—ì¤„)ê³??°ê²°?˜ì? ?Šì? ê³µë“¤??ì°¾ì•„ ë°˜í™˜?©ë‹ˆ?? (y,x) ??ëª©ë¡
std::vector<std::pair<int, int>> InGameScene::FindFloatingBalls()
{
    std::vector<std::pair<int, int>> floating = {};
    std::vector<std::vector<bool>> visited(ROWS, std::vector<bool>(COLS, false));
    std::queue<std::pair<int, int>> q = {};

    // 1) ë§??—ì¤„?ì„œ ê³µì´ ?ˆëŠ” ì¹¸ë“¤???œìž‘?ìœ¼ë¡??ì— ?½ìž…
    for (int y = 0; y < COLS; ++y) {
        if (board[DescentCount][y].ball != nullptr && board[DescentCount][y].ball->GetBallState()==eBallState::Idle)
        {           // ê³µì´ ?ˆëŠ” ì¹¸ë§Œ ?œìž‘
            visited[DescentCount][y] = true;
            q.push({ DescentCount, y });               // (y,x)
        }
    }

    // 2) BFS: ê³µì´ ?ˆëŠ” ì¹¸ë“¤?¼ë¦¬ë§??°ê²°???ìƒ‰
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

    // 3) ë°©ë¬¸?˜ì? ?Šì? ê³?=ë£¨íŠ¸ ë¯¸ì—°ê²?ë§??˜ì§‘
    for (int x = DescentCount; x < ROWS; ++x) {
        for (int y = 0; y < COLS; ++y) {
            if (board[x][y].ball != nullptr && !visited[x][y]) {
                floating.push_back({ x, y });
            }
        }
    }

    return floating;
}

void InGameScene::Start()
{
        playerarrow.Initialize(*renderer);
    NumVerticesArrow = sizeof(arrowVertices) / sizeof(FVertexSimple);
    arrowVertexBuffer = renderer->CreateVertexBuffer(arrowVertices, sizeof(arrowVertices));

    ShotBall = nullptr;

  BallQueue = std::queue<Ball*>(); // ?¤ë¥˜ê°€ ?˜ëŠ” ì¤?
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


   // ?„ì‹œ ?ˆë²¨(0,4)
   Ball* temp= new Ball;
   board[0][4].ball = temp;
   board[0][4].ball->Initialize(*renderer);
   board[0][4].ball->SetWorldPosition({ 0.0f, 0.89f, 0.0f });


    for (int i = 0; i < COLS; ++i)
        board[0][i].bEnable = true;
    board[0][4].bEnable = false;
    board[1][4].bEnable = true;
}

void InGameScene::Update(float deltaTime)
{
    DescentEventTime += TimeManager::GET_SINGLE()->GetDeltaTime();

    if (DescentEventTime >= 5.0f)
    {
       

        // start descent
        // change board
        DescentBoard();
        
        DescentCount++;

        // reset DescentEventTime
        DescentEventTime = 0.0f;
    }


    playerarrow.Update(*renderer);

    for (int i = DescentCount;i < ROWS; ++i)
    {
        for (int j = 0;j < COLS; ++j)
        {
            if (board[i][j].ball != nullptr)
            {
                board[i][j].ball->Update(*renderer);
            }
        }
    }

    for (int i = DescentCount;i < ROWS; ++i)
    {
        for (int j = 0;j < COLS; ++j)
        {
            if (board[i][j].ball != nullptr && board[i][j].ball->GetBallState() != eBallState::Idle)
            {
                if (board[i][j].ball->GetBallState() == eBallState::Fallen)
                {
                    FVector3 pos = board[i][j].ball->GetWorldPosition();
                    if (pos.x <= -1 || pos.x >= 1 || pos.y <= -1)
                    {
                        SAFE_DELETE(board[i][j].ball);
                        board[i][j].ball = nullptr;
                    }
                }
                else
                {
                    board[i][j].ball->SetBallState(eBallState::Fallen);
                    board[i][j].ball->SetIsGravity(true);
                }
            }

            board[i][j].bEnable = false;
        }
    }


    // 1ì°?true ì²˜ë¦¬
    for (int i = DescentCount;i < ROWS; ++i)
    {
        for (int j = 0;j < COLS; ++j)
        {
            if (board[i][j].ball != nullptr) // ê³µì´ ?ˆë‹¤ë©?
            {
                const int dx[4] = { 1, -1, 0, 0 };
                const int dy[4] = { 0, 0, 1, -1 };

                //? íš¨??ê²€??
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

    // 2ì°?ë²½ë©´ì²˜ë¦¬
    for (int i = 0;i < COLS; ++i)
    {
        if (board[DescentCount][i].ball == nullptr)
        {
            board[DescentCount][i].bEnable = true;
        }
    }

	BallQueue.back()->Update(*renderer);
	BallQueue.front()->Update(*renderer);

    if (ShotBall != nullptr)
    {
        ShotBall->Update(*renderer);
    }
}


void InGameScene::LateUpdate(float deltaTime)
{
    if (ShotBall == nullptr)
    {
        int bGameClear = 2;

        // ê²Œìž„ ?¤ë²„ ê²€?¬í•˜ê¸?
        for (int i = 0;i < COLS;++i)
        {
            if (board[6][i].ball != nullptr)
            {
                bGameClear = 1;
                break;
            }
        }
        if (bGameClear == 1)
        {
            SceneManager::GetInstance()->SetScene(new GameOverScene(hWND, renderer));
        }

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
        if (bGameClear == 2)
        {
            SceneManager::GetInstance()->SetScene(new ClearScene(hWND, renderer));
        }

        return;
    }
    FVector3 ShotBallPosition = ShotBall->GetWorldPosition();
    FVector3 ShotBallVelocity = ShotBall->GetVelocity();
    int dy = std::round((ShotBallPosition.x + 1) / 2.0f * static_cast<float>(COLS - 1)); // ?´ì°¨??ë°°ì—´??ê°€ë¡?(ì²?ë²ˆì§¸)
    int dx = ROWS - 1 -std::round((ShotBallPosition.y + 1) / 2.0f * static_cast<float>(ROWS - 1)); // ?´ì°¨??ë°°ì—´???¸ë¡œ (??ë²ˆì§¸)
    FVector3 NewVector = { (dy - 4) * 0.22F, (dx - 4) * - 0.22f, 0.0f };

    //?¼ìª½ ë²½ì´???¿ì•˜?„ë•Œ
    if (ShotBall && (ShotBallPosition.x - 0.11f <= -1.0f * ScreenUtil::GetAspectRatio()))
    {
        ShotBall->SetWorldPosition({ -ScreenUtil::GetAspectRatio() + 0.11f, ShotBallPosition.y, ShotBallPosition.z });
        ShotBall->SetVelocity({ -ShotBallVelocity.x, ShotBallVelocity.y, ShotBallVelocity.z });
    }

    //?¤ë¥¸ìª?ë²½ì´???¿ì•˜?„ë•Œ
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
            // ????ì¢???4êµ°ë° ê²€??
            const int nx = dx + cx[i];
            const int ny = dy + cy[i];
            
            if (IsInRange(nx, ny))
            {
                if (board[nx][ny].bEnable == false && board[nx][ny].ball == nullptr)
                    board[nx][ny].bEnable = true;
            }
        }

         std::vector<std::pair<int, int>> ResultDieVector = FindSameColorBalls({ dx,dy }, board[dx][dy].ball->GetBallColor());
         if (ResultDieVector.size() >= 3)
        {
            for (auto& pos : ResultDieVector)
            {
                board[pos.first][pos.second].ball->SetBallState(eBallState::Die);
            }
            
        }
        // 2 ?ë²ˆì§??Œê´´
        std::vector<std::pair<int, int>> ResultFallenVector = FindFloatingBalls();

        for (auto& pos : ResultFallenVector)
        {
            board[pos.first][pos.second].ball->SetBallState(eBallState::Die);
        }

        // 0: ?¤ì‹œ?˜ê¸°, 1:ê²Œìž„?¤ë²„, 2:ê²Œìž„?´ë¦¬??
        int bGameClear = 2;

        // ê²Œìž„ ?¤ë²„ ê²€?¬í•˜ê¸?
        for (int i = 0;i < COLS;++i)
        {
            if (board[6][i].ball != nullptr)
            {
                //?¬ê¸°??ê²Œìž„?¤ë²„ ?¤í–‰
                bGameClear = 1;
            }
        }

        

        //ê²Œìž„ ?´ë¦¬??ê²€??
        for (int i = DescentCount;i < ROWS; ++i)
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
    // 0: ?¤ì‹œ?˜ê¸°, 1:ê²Œìž„?¤ë²„, 2:ê²Œìž„?´ë¦¬??
        // 0: °ÔÀÓÁö¼Ó 1: °ÔÀÓ ¿À¹ö 2: °ÔÀÓ Å¬¸®¾î

    
}

void InGameScene::OnMessage(MSG msg)
{
    if (msg.message == WM_KEYDOWN)
    {
        if (msg.wParam == VK_LEFT)
        {
            // ì¢ŒíšŒ??
            playerarrow.SetDegree(-rotationDelta);
        }
        else if (msg.wParam == VK_RIGHT)
        {
            // ?°íšŒ??
            playerarrow.SetDegree(rotationDelta);
        }
        else if (msg.wParam == VK_SPACE)
        {
            // ì´ì•Œ?ì„±
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
                ball->Update(*renderer); // ball 첫 프레임에 커지는 거 방지
                BallQueue.push(ball);

                float speed = 0.02f; // ?í•˜???ë„ ê°?
                ShotBall->SetVelocity({ -cosf(DirectX::XMConvertToRadians(playerarrow.GetDegree() + 90)) * speed,
                                        sinf(DirectX::XMConvertToRadians(playerarrow.GetDegree() + 90)) * speed,
                                        0.0f });
              
            
                //ShotBall->SetVelocity( 
            }
            

        }
    }
}

void InGameScene::OnGUI(HWND hWND)
{
    ImGui::Text("Hello Jungle World!");
    if (ImGui::Button("Quit this app"))
    {
        // ?„ìž¬ ?ˆë„?°ì— Quit ë©”ì‹œì§€ë¥?ë©”ì‹œì§€ ?ë¡œ ë³´ëƒ„
        PostMessage(hWND, WM_QUIT, 0, 0);
    }
}

void InGameScene::OnRender()
{
    playerarrow.Render(*renderer);

    //?„ì²´ ë°°ì—´ ?Œë”
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

    //ë°œì‚¬ì´ì•Œ ?Œë”
    if (ShotBall != nullptr)
    {
        ShotBall->Render(*renderer);
    }
}

void InGameScene::Shutdown()
{
    while (!BallQueue.empty()) {
        Ball* b = BallQueue.front(); // ë§????”ì†Œ ê°€?¸ì˜¤ê¸?
        BallQueue.pop();            // ë§????”ì†Œ ?œê±°

        delete b;
    }
}

void InGameScene::DescentBoard()
{
    for (int i = ROWS - 1; i > DescentCount;--i)
    {
        for (int j = 0; j < COLS; ++j)
        {
            board[i][j].ball = board[i - 1][j].ball;
            board[i][j].bEnable = board[i - 1][j].bEnable;

            if (board[i-1][j].ball != nullptr)
            {
                FVector3 TempVector = board[i - 1][j].ball->GetWorldPosition();
                board[i][j].ball->SetWorldPosition({TempVector.x, TempVector.y - 0.22f, TempVector.z});
            }

        }
    }

    for (int j = 0; j < COLS; ++j)
    {
        board[DescentCount][j].ball = nullptr;
        board[DescentCount][j].bEnable = false;
    }


    // stair block algorithm

}
