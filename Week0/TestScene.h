#pragma once
#include "SceneManager.h"
#include "PlayerArrow.h"

class TestScene : public Scene {
private:
	float rotationDeg = 0.0f;
	float rotationDelta = 5.0f;
	INT NumVerticesArrow;
	ID3D11Buffer* arrowVertexBuffer;

	PlayerArrow playerarrow;

public:
	TestScene(Renderer * renderer): Scene(renderer){}

	void Start() override;
	void Update(float deltaTime) override;
	void OnGUI(HWND hWND) override;
	void OnRender() override;
	void Shutdown() override;
};