#pragma once
#include "Scene.h"

class SceneManager
{
private:
	static SceneManager* sInstance;
	Scene* currentScene;

	SceneManager();
	SceneManager(const SceneManager& other) = delete;
	SceneManager& operator=(const SceneManager& other) = delete;

public:
	~SceneManager();

	void SetScene(Scene* scene);
	void Shutdown();
	void Update(float deltaTime);
	void LateUpdate(float deltaTime);
	void OnMessage(MSG msg);
	void OnGUI(HWND hWND);
	void OnRender();

	static SceneManager* GetInstance();
	static void DestroyInstance();


	//getter
	inline int GetTotalScore() const { return TotalScore; }

	//setter
	inline void ResetTotalScore() { TotalScore = 0; }

	inline void AddScore(int InScore)
	{
		TotalScore += InScore;
	}

private:
	int  TotalScore{};


};
