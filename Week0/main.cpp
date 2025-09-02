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



// Ball �Լ� ����
//void UBall::Initialize(const URenderer& InRenderer)
//{
//    Radius = RandomUtil::CreateRandomFloat(0.05f, 0.2f);
//    WorldPosition = FVector3(RandomUtil::CreateRandomFloat(-1.0f + Radius, 1.0f - Radius), RandomUtil::CreateRandomFloat(-1.0f + Radius, 1.0f - Radius), 0.0f);
//    Velocity = FVector3(RandomUtil::CreateRandomFloat(-0.01f, 0.01f), RandomUtil::CreateRandomFloat(-0.01f, 0.01f), 0.0f);
//    Mass = Radius * Radius;
//    CreateConstantBuffer(InRenderer);
//}
//
//void UBall::Update(URenderer& Renderer)
//{
//    Renderer.UpdateConstant(WorldPosition, Radius, ConstantBuffer);
//}
//
//void UBall::Render(URenderer& Renderer)
//{
//    Renderer.SetConstantBuffer(ConstantBuffer);
//    Renderer.Render();
//
//    //ResetOffset();
//}   
//
//void UBall::Release()
//{
//    ConstantBuffer->Release();
//    ConstantBuffer = nullptr;
//}
//
//void UBallNode::Initialize()
//{
//    
//}
//
//void UBallNode::Release()
//{
//    delete Ball;
//    //Ball->Release();
//}
//
//
//
//void UBallList::MoveAll()
//{
//    CollisionUtil::PerfectElasticCollision(*this);
//}
//
//
//void UBallList::DoRenderAll(URenderer& InRenderer)
//{
//    for (UBallNode* curr = HeadNode; curr; curr = curr->GetNextNode())
//    {
//        if (curr->GetUBall() == nullptr) continue;
//        curr->GetUBall()->Render(InRenderer);
//    }
//}
//
//
//void UBallList::Add(const URenderer& InRenderer) {
//    UBall* NewBall = new UBall;
//    NewBall->Initialize(InRenderer);
//
//    UBallNode* NewNode = new UBallNode;
//    NewNode->SetUBall(NewBall);
//
//    if (BallCount == 0) {
//        HeadNode->SetNextNode(NewNode);
//        NewNode->SetPrevNode(HeadNode);
//        NewNode->SetNextNode(TailNode);
//        TailNode->SetPrevNode(NewNode);
//    }
//    else {
//        TailNode->GetPrevNode()->SetNextNode(NewNode);
//        NewNode->SetPrevNode(TailNode->GetPrevNode());
//        NewNode->SetNextNode(TailNode);
//        TailNode->SetPrevNode(NewNode);
//    }
//    ++BallCount;
//}
//
//
//void UBallList::Delete(URenderer& Renderer)
//{
//    UBallNode* TargetNode = HeadNode->GetNextNode();
//
//    int TargetNumber = rand() % BallCount;
//
//    for (int i = 0; i < TargetNumber; ++i)
//    {
//        TargetNode = TargetNode->GetNextNode();
//    }
//
//    TargetNode->GetPrevNode()->SetNextNode(TargetNode->GetNextNode());
//    TargetNode->GetNextNode()->SetPrevNode(TargetNode->GetPrevNode());
//
//    delete TargetNode;
//
//    BallCount--;
//}
//
//
//void UBallList::Initialize()
//{
//    UBallNode* NewHeadNode = new UBallNode;
//    UBallNode* NewTailNode = new UBallNode;
//
//    HeadNode = NewHeadNode;
//    TailNode = NewTailNode;
//
//    HeadNode->SetNextNode(TailNode);
//    TailNode->SetPrevNode(HeadNode);
//}
//
//void UBallList::UpdateAll(URenderer& Renderer)
//{
//    for (UBallNode* curr = HeadNode; curr; curr = curr->GetNextNode())
//    {
//        if (curr->GetUBall() == nullptr) continue;
//        curr->GetUBall()->Update(Renderer);
//    }
//}
//
//
//void UBallList::Release()
//{
//    UBallNode* curr = HeadNode;
//    while (curr)
//    {
//        UBallNode* next = curr->GetNextNode();
//        delete curr;
//        curr = next;
//    }
//    HeadNode = TailNode = nullptr;
//}
//
//
//void CollisionUtil::BallMove(UBallList* BallList)
//{
//    auto iterStartNode = BallList->GetHead();
//
//    if (!iterStartNode)return;
//    
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // pStartBall �� valid���� üũ
//        if (!pNowBall)continue;
//        
//        // ���� ���� ��ġ�� �ӵ��� ������
//        FVector3 BallVelocity = pNowBall->GetVelocity();
//        FVector3 Offset = BallVelocity;
//
//        // ũ�� ��������
//        float Ballradius = pNowBall->GetRadius();
//
//        
//        
//              
//      
//
//        // ���� ��ġ�� ������� �� ó�� 
//        if (Offset.x < GLeftBorder + Ballradius)
//        {
//            BallVelocity.x *= -1.0f;
//        }
//        if (Offset.x > GRightBorder - Ballradius)
//        {
//            BallVelocity.x *= -1.0f;
//        }
//        if (Offset.y < GTopBorder + Ballradius)
//        {
//            BallVelocity.y *= -1.0f;
//        }
//        if (Offset.y > GBottomBorder - Ballradius)
//        {
//            BallVelocity.y *= -1.0f;
//        }
//        //pNowBall->SetLocation(NextBallLocation);
//        pNowBall->SetVelocity(BallVelocity);
//    } 
//}
//
//void CollisionUtil::CollisionBallMove(UBallList* BallList)
//{
//    auto iterNowNode = BallList->GetHead();
//
//    if (!iterNowNode) return;
//    
//    for (; iterNowNode != nullptr; iterNowNode = iterNowNode->GetNextNode())
//    {
//        UBall* pBallA = iterNowNode->GetUBall();
//
//        auto iterNextNode = iterNowNode->GetNextNode();
//        if (!iterNextNode) continue;
//
//
//        for (; iterNextNode != nullptr; iterNextNode = iterNextNode->GetNextNode())
//        {
//            UBall* pBallB = iterNextNode->GetUBall();
//
//            // �ΰ��� ���� valid���� üũ
//            if (pBallA && pBallB)
//            {
//                float dist = CalculateUtil::Length(CalculateUtil::operator-(pBallA->GetWorldPosition(), pBallB->GetWorldPosition()));
//                if (dist <= pBallA->GetRadius() + pBallB->GetRadius())
//                {
//                    CollisionUtil::ResolveCollision(pBallA, pBallB);
//                }
//            }
//        }   
//    }   
//}
//
//void ResolveGravityCollision(UBall* pStartBall, UBall* pAnotherBall)
//{
//    FVector3 posA = pStartBall->GetWorldPosition();
//    FVector3 posB = pAnotherBall->GetWorldPosition();
//
//    FVector3 velA = pStartBall->GetVelocity();
//    FVector3 velB = pAnotherBall->GetVelocity();
//
//    float massA = pStartBall->GetMass();
//    float massB = pAnotherBall->GetMass();
//
//    // �� ���� ��ġ���̸� ���
//    FVector3 delta(posA.x - posB.x, posA.y - posB.y, 0);
//    float distSquared = delta.x * delta.x + delta.y * delta.y;
//    if (distSquared == 0.0f) return;
//
//    float distance = sqrtf(distSquared);
//    FVector3 normal(delta.x / distance, delta.y / distance, 0); // normalize = n
//
//    // ������ �浹�ÿ��� ��ġ����
//    float overlap = pStartBall->GetRadius() + pAnotherBall->GetRadius() - distance;
//    if (overlap > 0.0f)
//    {
//       FVector3 correction = CalculateUtil::operator*(normal, (overlap / 2.0f));
//       posA = CalculateUtil::operator+(posA, correction);
//       posB = CalculateUtil::operator-(posB, correction);
//       pStartBall->SetWorldPosition(posA);
//       pAnotherBall->SetWorldPosition(posB);
//    }
//
//    FVector3 relVel(velA.x - velB.x, velA.y - velB.y, 0); // ��� �ӵ�
//
//    // ��� �ӵ��� �浹 ���� ����
//    float relSpeed = CalculateUtil::Dot(relVel, normal);
//
//    // �ʹ� ������ ����
//    const float MinimumRelativeSpeed = 0.001f;
//    if (relSpeed >= 0.0f || fabs(relSpeed) < MinimumRelativeSpeed) return;
//        
//    // ź�� �浹 ���� 
//    float impulse = (-(1 + GElastic) * relSpeed) / ((1 / massA) + (1 / massB)); // = j
//
//    FVector3 impulseA(normal.x * (impulse / massA), normal.y * (impulse / massA), 0);
//    FVector3 impulseB(normal.x * (impulse / massB), normal.y * (impulse / massB), 0);
//
//    FVector3 NewVelocityA(velA.x + impulseA.x, velA.y + impulseA.y, 0);
//    FVector3 NewVelocityB(velB.x - impulseB.x, velB.y - impulseB.y, 0);
//
//    if (CalculateUtil::Length(NewVelocityA) < 0.004f) NewVelocityA = FVector3(0, 0, 0);
//    if (CalculateUtil::Length(NewVelocityB) < 0.004f) NewVelocityB = FVector3(0, 0, 0);
//
//
//    pStartBall->SetNextVelocity(NewVelocityA);
//    pAnotherBall->SetNextVelocity(NewVelocityB);
//}
//
//void CollisionUtil::GravityBallMove(UBallList*  BallList)
//{
//    if (!BallList) return;
//
//    auto iterStartNode = BallList->GetHead();
//
//    if (!iterStartNode) return;
//
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // pStartBall �� valid���� üũ
//        if (!pNowBall)continue;
//
//        pNowBall->SetNextVelocity(pNowBall->GetVelocity());
//    }
//
//    iterStartNode = BallList->GetHead();
//
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();  
//
//        // pStartBall �� valid���� üũ
//        if (!pNowBall)continue;
//
//        // ũ�� ��������
//        const float Ballradius = pNowBall->GetRadius();
//
//        auto iterNextNode = iterStartNode->GetNextNode();
//        if (!iterNextNode) continue;
//
//
//        // �̰����� ���� �浹 �������� nextgravityvelocity�� �������� ������
//        for (; iterNextNode != nullptr; iterNextNode = iterNextNode->GetNextNode())
//        {
//            UBall* pAnotherBall = iterNextNode->GetUBall();
//
//            // �ΰ��� ���� valid���� üũ
//            if ((!pNowBall || !pAnotherBall))continue;
//            
//            
//            
//            float dist = CalculateUtil::Length(CalculateUtil::operator-(pNowBall->GetWorldPosition(), pAnotherBall->GetWorldPosition()));
//            if (dist <= pNowBall->GetRadius() + pAnotherBall->GetRadius())
//            {
//                ResolveGravityCollision(pNowBall, pAnotherBall);           
//            }
//        }            
//    }
//
//    // �ѹ��� ��ġ�� ����
//    iterStartNode = BallList->GetHead();
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // ũ�� ��������
//        float Ballradius = pNowBall->GetRadius();
//
//        // pStartBall �� valid���� üũ
//        if (!pNowBall)continue;
//
//        // ���� �浹 �ӵ� ���� �� ��ġ
//        FVector3 NextBallLocation = CalculateUtil::operator+(CalculateUtil::operator+(pNowBall->GetWorldPosition(), pNowBall->GetNextVelocity()), FVector3(0, -GGravityValue * 0.5f, 0));
//        pNowBall->SetNextVelocity({ pNowBall->GetNextVelocity().x, pNowBall->GetNextVelocity().y - GGravityValue, 0 });
//
//        // �������
//        if (NextBallLocation.x < GLeftBorder + Ballradius)
//        {
//            NextBallLocation.x = GLeftBorder + Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.x *= -GElastic;
//
//            if (fabs(Tempvelocity.x) < 0.005f) Tempvelocity.x = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//
//        }
//        if (NextBallLocation.x > GRightBorder - Ballradius)
//        {
//            NextBallLocation.x = GRightBorder - Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.x *= -GElastic;
//
//            if (fabs(Tempvelocity.x) < 0.005f) Tempvelocity.x = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        if (NextBallLocation.y < GTopBorder + Ballradius)
//        {
//            NextBallLocation.y = GTopBorder + Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.y *= -GElastic;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        if (NextBallLocation.y > GBottomBorder - Ballradius)
//        {
//            NextBallLocation.y = GBottomBorder - Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.y *= -GElastic;
//
//            if (fabs(Tempvelocity.y) < 0.05f) Tempvelocity.y = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        pNowBall->SetWorldPosition(NextBallLocation);
//        pNowBall->SetVelocity(pNowBall->GetNextVelocity());
//    }
//}
//
//void CollisionUtil::ResolveCollision(UBall* OutBallA, UBall* OutBallB)
//{
//    FVector3 posA = OutBallA->GetWorldPosition();
//    FVector3 posB = OutBallB->GetWorldPosition();
//
//    FVector3 velA = OutBallA->GetVelocity();
//    FVector3 velB = OutBallB->GetVelocity();
//
//    float massA = OutBallA->GetMass();
//    float massB = OutBallB->GetMass();
//
//    // ||p1 - p2||
//    float distance = CalculateUtil::Length(CalculateUtil::operator-(posA, posB));
//
//    // �븻��������
//    FVector3 normalizedvector = CalculateUtil::operator-(posA, posB);
//    normalizedvector.x /= distance;
//    normalizedvector.y /= distance;
//
//
//
//    FVector3 relativeVel = CalculateUtil::operator-(velA, velB);
//
//
//    float velAlongNormal = CalculateUtil::Dot(relativeVel, normalizedvector);
//
//    // �浹�� ���� �������θ� �Ͼ��
//    // ���� ���ӵ��� �븻���Ϳ� ���翵�Ѵ�
//    if (velAlongNormal > 0.0f || fabs(velAlongNormal) < 0.0001f)
//    {
//        return;
//    }
//
//    float invMassA = 1.0f / massA;
//    float invMassB = 1.0f / massB;
//
//    float j = 0.0f;
//    if (bIsGravity)
//    {
//        j = -(1 + GElastic) * velAlongNormal / (invMassA + invMassB);
//    }
//    else
//    {
//        j = -(2) * velAlongNormal / (invMassA + invMassB);
//    }
//
//    FVector3 impulse = CalculateUtil::operator*(normalizedvector, j);
//
//    FVector3 newVelA = CalculateUtil::operator+(velA, CalculateUtil::operator*(impulse, invMassA));
//    FVector3 newVelB = CalculateUtil::operator-(velB, CalculateUtil::operator*(impulse, invMassB));
//
//    OutBallA->SetVelocity(newVelA);
//    OutBallB->SetVelocity(newVelB);
//
//    // ħ�� ����
//   float penetration = OutBallA->GetRadius() + OutBallB->GetRadius() - distance;
//   if (penetration > 0.0f)
//   {
//       float correctionRatio = 0.5f;
//       FVector3 correction = CalculateUtil::operator*(normalizedvector, penetration * correctionRatio);
//   
//       OutBallA->SetWorldPosition(CalculateUtil::operator+(posA, correction));
//       OutBallB->SetWorldPosition(CalculateUtil::operator-(posB, correction));
//   }
//
//
//}
//
//void CollisionUtil::PerfectElasticCollision(UBallList& BallList)
//{
//    auto iterNowNode = BallList.GetHead()->GetNextNode();
//
//    for (; iterNowNode != nullptr; iterNowNode = iterNowNode->GetNextNode())
//    {
//        auto iterNextNode = iterNowNode->GetNextNode();
//    
//        for (; iterNextNode != nullptr; iterNextNode = iterNextNode->GetNextNode())
//        {
//            UBall* pBallA = iterNowNode->GetUBall();
//            UBall* pBallB = iterNextNode->GetUBall();
//    
//            // �ΰ��� ���� valid���� üũ
//            if (pBallA == nullptr || pBallB == nullptr)
//            {
//                continue;
//            }
//            float dist = CalculateUtil::Length(CalculateUtil::operator-(pBallA->GetWorldPosition(), pBallB->GetWorldPosition()));
//            if (dist <= pBallA->GetRadius() + pBallB->GetRadius())
//            {
//                CollisionUtil::ResolveCollision(pBallA, pBallB);
//            }
//    
//        }
//    }
//
//    iterNowNode = BallList.GetHead()->GetNextNode();
//
//    while (iterNowNode != BallList.GetTail())
//    {
//        UBall* pBallA = iterNowNode->GetUBall();
//
//        // ���� ���� ��ġ�� �ӵ��� ������
//        FVector3 BallVelocity = pBallA->GetVelocity();
//        FVector3 Position = pBallA->GetWorldPosition();
//
//        // �߷� ���ӵ� ����
//        if (bIsGravity)
//        {
//            BallVelocity.y -= GGravityValue;
//        }
//        
//
//
//
//        FVector3 NewPosition = CalculateUtil::operator+(Position, BallVelocity);
//
//
//        // ũ�� ��������
//        float Ballradius = pBallA->GetRadius();
//
//
//        // ���� ��ġ�� ������� �� ó�� 
//        if (NewPosition.x < GLeftBorder + Ballradius)
//        {
//            NewPosition.x = GLeftBorder + Ballradius;
//            if (bIsGravity)
//            {
//                BallVelocity.x *= -GElastic;
//            }
//            else
//            {
//                BallVelocity.x *= -1.0f;
//            }
//        }
//
//        if (NewPosition.x > GRightBorder - Ballradius)
//        {
//            NewPosition.x = GRightBorder - Ballradius;
//            if (bIsGravity)
//            {
//                BallVelocity.x *= -GElastic;
//            }
//            else
//            {
//                BallVelocity.x *= -1.0f;
//            }
//        }
//
//        if (NewPosition.y > GTopBorder - Ballradius)
//        {
//            NewPosition.y = GTopBorder - Ballradius;
//            if (bIsGravity)
//            {
//                BallVelocity.y *= -GElastic;
//            }
//            else
//            {
//                BallVelocity.y *= -1.0f;
//            }
//        }
//
//        if (NewPosition.y < GBottomBorder + Ballradius)
//        {
//            NewPosition.y = GBottomBorder + Ballradius;
//            if (bIsGravity)
//            {
//                BallVelocity.y *= -GElastic;
//            }
//            else
//            {
//                BallVelocity.y *= -1.0f;
//            }
//        }
//
//
//        //if (fabsf(BallVelocity.x) < 0.0005f)
//        //    BallVelocity.x = 0.0f;
//        //if (fabsf(BallVelocity.y) < 0.0005f)
//        //    BallVelocity.y = 0.0f;
//
//        pBallA->SetVelocity(BallVelocity);
//        pBallA->SetWorldPosition(NewPosition);
//
//        iterNowNode = iterNowNode->GetNextNode();
//    }
//}



//void CollisionUtil::GravityBallMove(UBallList* BallList)
//{
//    if (!BallList) return;
//
//    auto iterStartNode = BallList->GetHead();
//
//    if (!iterStartNode) return;
//
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // pStartBall �� valid���� üũ
//        if (!pNowBall)continue;
//
//        pNowBall->SetNextVelocity(pNowBall->GetVelocity());
//    }
//
//    iterStartNode = BallList->GetHead();
//
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // pStartBall �� valid���� üũ
//        if (!pNowBall)continue;
//
//        // ũ�� ��������
//        const float Ballradius = pNowBall->GetRadius();
//
//        auto iterNextNode = iterStartNode->GetNextNode();
//        if (!iterNextNode) continue;
//
//
//        // �̰����� ���� �浹 �������� nextgravityvelocity�� �������� ������
//        for (; iterNextNode != nullptr; iterNextNode = iterNextNode->GetNextNode())
//        {
//            UBall* pAnotherBall = iterNextNode->GetUBall();
//
//            // �ΰ��� ���� valid���� üũ
//            if ((!pNowBall || !pAnotherBall))continue;
//
//
//
//            float dist = CalculateUtil::Length(CalculateUtil::operator-(pNowBall->GetLocation(), pAnotherBall->GetLocation()));
//            if (dist <= pNowBall->GetRadius() + pAnotherBall->GetRadius())
//            {
//                ResolveGravityCollision(pNowBall, pAnotherBall);
//            }
//        }
//    }
//
//    // �ѹ��� ��ġ�� ����
//    iterStartNode = BallList->GetHead();
//    for (; iterStartNode != nullptr; iterStartNode = iterStartNode->GetNextNode())
//    {
//        UBall* pNowBall = iterStartNode->GetUBall();
//
//        // ũ�� ��������
//        float Ballradius = pNowBall->GetRadius();
//
//        // pStartBall �� valid���� üũ
//        if (!pNowBall)continue;
//
//        // ���� �浹 �ӵ� ���� �� ��ġ
//        FVector3 NextBallLocation = CalculateUtil::operator+(CalculateUtil::operator+(pNowBall->GetLocation(), pNowBall->GetNextVelocity()), FVector3(0, -GGravityValue * 0.5f, 0));
//        pNowBall->SetNextVelocity({ pNowBall->GetNextVelocity().x, pNowBall->GetNextVelocity().y - GGravityValue, 0 });
//
//        // �������
//        if (NextBallLocation.x < GLeftBorder + Ballradius)
//        {
//            NextBallLocation.x = GLeftBorder + Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.x *= -GElastic;
//
//            if (fabs(Tempvelocity.x) < 0.005f) Tempvelocity.x = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//
//        }
//        if (NextBallLocation.x > GRightBorder - Ballradius)
//        {
//            NextBallLocation.x = GRightBorder - Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.x *= -GElastic;
//
//            if (fabs(Tempvelocity.x) < 0.005f) Tempvelocity.x = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        if (NextBallLocation.y < GTopBorder + Ballradius)
//        {
//            NextBallLocation.y = GTopBorder + Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.y *= -GElastic;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        if (NextBallLocation.y > GBottomBorder - Ballradius)
//        {
//            NextBallLocation.y = GBottomBorder - Ballradius;
//
//            FVector3 Tempvelocity{ pNowBall->GetNextVelocity() };
//            Tempvelocity.y *= -GElastic;
//
//            if (fabs(Tempvelocity.y) < 0.05f) Tempvelocity.y = 0.0f;
//
//            pNowBall->SetNextVelocity(Tempvelocity);
//        }
//        pNowBall->SetLocation(NextBallLocation);
//        pNowBall->SetVelocity(pNowBall->GetNextVelocity());
//    }
//}
