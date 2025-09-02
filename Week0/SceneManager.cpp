#include "SceneManager.h"

// 정적 멤버 정의
SceneManager* SceneManager::sInstance = nullptr;

SceneManager::SceneManager() : currentScene(nullptr)
{
}

SceneManager::~SceneManager()
{
	Shutdown();
}

void SceneManager::SetScene(Scene* scene)
{
	if (currentScene == scene) return;  // 같은 포인터 체크

	if (currentScene) {
		currentScene->Shutdown();
		delete currentScene;
	}

	currentScene = scene;

	if (currentScene) {
		currentScene->Start();
	}
}

void SceneManager::Shutdown()
{
	if (currentScene) {
		currentScene->Shutdown();
		delete currentScene;
		currentScene = nullptr;
	}
}

void SceneManager::Update(float deltaTime)
{
	if (currentScene)
	{
		currentScene->Update(deltaTime);
	}
}

void SceneManager::OnGUI(HWND hWND)
{
	if (currentScene)
	{
		currentScene->OnGUI(hWND);
	}
}

void SceneManager::OnRender()
{
	if (currentScene)
	{
		currentScene->OnRender();
	}
}

SceneManager* SceneManager::GetInstance()
{
	if (sInstance == nullptr)
	{
		sInstance = new SceneManager();
	}
	return sInstance;
}

void SceneManager::DestroyInstance()
{
	if (sInstance)
	{
		delete sInstance;
		sInstance = nullptr;
	}
}