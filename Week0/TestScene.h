#pragma once
#include "SceneManager.h"
#include "PlayerArrow.h"
#include "Ball.h"


struct Board
{
	Ball* ball = nullptr;
	bool bEnable = false;
};


class TestScene : public Scene {
private:
	float rotationDeg = 0.0f;
	float rotationDelta = 0.3f;
	INT NumVerticesArrow;
	ID3D11Buffer* arrowVertexBuffer;

	PlayerArrow playerarrow;

	Board board[ROWS][COLS] = {};

	Ball* ShotBall{};

	inline bool IsInRange(const int x, const int y) const;

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