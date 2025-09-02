#pragma once
#pragma once
#include "SceneManager.h"

class TestScene2 : public Scene {
private:

public:
	TestScene2(Renderer* renderer) : Scene(renderer) {}

	void Start() override;
	void Update(float deltaTime) override;
	void LateUpdate(float deltaTime) override;
	void OnMessage(MSG msg) override;
	void OnGUI(HWND hWND) override;
	void OnRender() override;
	void Shutdown() override;
};