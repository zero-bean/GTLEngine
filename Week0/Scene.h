#pragma once
#include "pch.h"
#include "Renderer.h"

class Scene
{
protected:
	Renderer* renderer;
public:
   	Scene(Renderer* newRenderer) { renderer = std::move(newRenderer); }
	virtual ~Scene() = default;

	virtual void Start() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void LateUpdate(float deltaTime) = 0;
	virtual void OnGUI(HWND hWND) = 0;
	virtual void OnMessage(MSG msg) = 0;
	virtual void OnRender() = 0;
	virtual void Shutdown() = 0;
};