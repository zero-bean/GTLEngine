#pragma once
#pragma once
#include "SceneManager.h"
#include "Button.h"
#include "Image.h"

class TitleScene : public Scene {
private:
	Image* logo;
	Button* exitButton;
	Button* playButton;
	HWND* hWND;
public:
	TitleScene(HWND* newhWND, Renderer* newRenderer) : Scene(newRenderer) {
		hWND = newhWND;
	}

	void Start() override;
	void Update(float deltaTime) override;
	void LateUpdate(float deltaTime) override;
	void OnMessage(MSG msg) override;
	void OnGUI(HWND hWND) override;
	void OnRender() override;
	void Shutdown() override;
};