#pragma once
#include "SceneManager.h"
#include "PlayerArrow.h"
#include "Ball.h"

class TestScene : public Scene {
private:
	float rotationDeg = 0.0f;
	float rotationDelta = 5.0f;
	INT NumVerticesArrow;
	ID3D11Buffer* arrowVertexBuffer;

	PlayerArrow playerarrow;

	Ball* balls[ROWS][COLS] = {};

	Ball* ShotBall{};


public:
	TestScene(Renderer * renderer): Scene(renderer){}

	void Start() override;
	void Update(float deltaTime) override;
	void LateUpdate(float deltaTime) override;
	void OnMessage(MSG msg) override;
	void OnGUI(HWND hWND) override;
	void OnRender() override;
	void Shutdown() override;
};