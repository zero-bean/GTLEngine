#pragma once
#include "pch.h"

class Scene
{
protected:
	Renderer* renderer;
public:
	Scene(Renderer* newRenderer) { renderer = newRenderer; }
	virtual ~Scene() = default;

	virtual void Start() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void OnGUI(HWND hWND) = 0;
	virtual void OnRender() = 0;
	virtual void Shutdown() = 0;
};