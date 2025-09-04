#include "InGameScene.h"
#include "GameOverScene.h"

void GameOverScene::Start()
{
    logo = new Image(L"assets/gameover_team.png", { 0.75f, 1.0f });
      logo->Initialize(*renderer);
    logo->SetWorldPosition(FVector3(0, 0, 0));

    playButton = new Button(L"assets/reset.png", { 0.25f, 0.0625f });
    playButton->Initialize(*renderer);
    playButton->SetWorldPosition(FVector3(0, -0.2f, 0));
    playButton->SetCallback([this]() {
        SceneManager::GetInstance()->SetScene(new InGameScene(hWND, renderer));
        });
    exitButton = new Button(L"assets/exit.png", { 0.25f, 0.0625f });
    exitButton->Initialize(*renderer);
    exitButton->SetWorldPosition(FVector3(0, -0.4f, 0));
    exitButton->SetCallback([this]() {
        PostMessage(*hWND, WM_QUIT, 0, 0);
        });
}

void GameOverScene::Update(float deltaTime)
{
    logo->Update(*renderer);

    exitButton->Update(*renderer);
    playButton->Update(*renderer);
}

void GameOverScene::LateUpdate(float deltaTime)
{

}

void GameOverScene::OnMessage(MSG msg)
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

void GameOverScene::OnGUI(HWND hWND)
{
    ImGui::Text("Hello Jungle World!");
    if (ImGui::Button("Start Game"))
    {
        SceneManager::GetInstance()->SetScene(new InGameScene(&hWND, renderer));
    }
}

void GameOverScene::OnRender()
{
    logo->Render(*renderer);

    exitButton->Render(*renderer);
    playButton->Render(*renderer);
}

void GameOverScene::Shutdown()
{
    SAFE_DELETE(exitButton);
    SAFE_DELETE(playButton);
    SAFE_DELETE(logo);
}
