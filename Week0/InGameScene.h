#pragma once
#include "SceneManager.h"
#include "PlayerArrow.h"
#include "Ball.h"

struct Board
{
	Ball* ball = nullptr;
	bool bEnable = false;
};

class InGameScene : public Scene {
private:
	HWND* hWND;
	float rotationDeg = 0.0f;
	float rotationDelta = 0.3f;
	INT NumVerticesArrow;
	ID3D11Buffer* arrowVertexBuffer;

	PlayerArrow playerarrow;

	Board board[ROWS][COLS] = {};

	Ball* ShotBall{};
	std::queue<Ball*> BallQueue;

	inline bool IsInRange(const int x, const int y) const;



    // 해당 위치의 볼과 4-방향 "인접한 같은 색상의 볼"을 탐색하고 반환합니다.
	std::vector<std::pair<int, int>> FindSameColorBalls(const std::pair<int, int>& start, const eBallColor& color);


    // 낙하할 볼을 탐색하고 반환합니다.
    // 루트(맨 윗줄)과 연결되지 않은 공들을 찾아 반환합니다. (y,x) 쌍 목록
    std::vector<std::pair<int, int>> FindFloatingBalls();
    

public:
	InGameScene(HWND * hWnd, Renderer * newRenderer): Scene(newRenderer){
		hWND = hWnd;
	}

	void Start() override;
	void Update(float deltaTime) override;
	void LateUpdate(float deltaTime) override;
	void OnMessage(MSG msg) override;
	void OnGUI(HWND hWND) override;
	void OnRender() override;
	void Shutdown() override;
};