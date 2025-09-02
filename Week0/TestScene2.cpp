#include "TestScene.h"
#include "TestScene2.h"

void TestScene2::Start()
{
}

void TestScene2::Update(float deltaTime)
{
}

void TestScene2::LateUpdate(float deltaTime)
{
}

void TestScene2::OnMessage(MSG msg)
{
}

void TestScene2::OnGUI(HWND hWND)
{
    ImGui::Text("Hello Jungle World!");
    if (ImGui::Button("Start Game"))
    {
        SceneManager::GetInstance()->SetScene(new TestScene(renderer));
    }
}

void TestScene2::OnRender()
{
}

void TestScene2::Shutdown()
{
}
