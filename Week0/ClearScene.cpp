#include "InGameScene.h"
#include "ClearScene.h"

void ClearScene::Start()
{
    logo = new Image(L"assets/clear_team.png", { 0.75f, 1.0f });
    logo->Initialize(*renderer);
    logo->SetWorldPosition(FVector3(0, 0, 0));

    playButton = new Button(L"assets/reset.png", { 0.25f, 0.0625f });
    playButton->Initialize(*renderer);
    playButton->SetWorldPosition(FVector3(0, -0.2f, 0));
    playButton->SetCallback([this]() {
        SceneManager::GetInstance()->SetScene(new InGameScene(renderer));
        });
    exitButton = new Button(L"assets/exit.png", { 0.25f, 0.0625f });
    exitButton->Initialize(*renderer);
    exitButton->SetWorldPosition(FVector3(0, -0.4f, 0));
    exitButton->SetCallback([this]() {
        PostMessage(*hWND, WM_QUIT, 0, 0);
        });
}

void ClearScene::Update(float deltaTime)
{
    logo->Update(*renderer);

    exitButton->Update(*renderer);
    playButton->Update(*renderer);
}

void ClearScene::LateUpdate(float deltaTime)
{

}

void ClearScene::OnMessage(MSG msg)
{
    if (msg.message == WM_LBUTTONDOWN)
    {
        int mouseX = LOWORD(msg.lParam);
        int mouseY = HIWORD(msg.lParam);

        if (exitButton->IsOverlapWithMouse(mouseX, mouseY))
        {
            exitButton->OnClick();
        }

        if (playButton->IsOverlapWithMouse(mouseX, mouseY))
        {
            playButton->OnClick();
        }
    }
}

void ClearScene::OnGUI(HWND hWND)
{
    ImGui::Text("Hello Jungle World!");
    if (ImGui::Button("Start Game"))
    {
        SceneManager::GetInstance()->SetScene(new InGameScene(renderer));
    }
}

void ClearScene::OnRender()
{
    logo->Render(*renderer);

    exitButton->Render(*renderer);
    playButton->Render(*renderer);
}

void ClearScene::Shutdown()
{
}
