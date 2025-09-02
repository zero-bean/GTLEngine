#pragma once
#include "pch.h"

extern class URenderer;

class Scene
{
protected:
	URenderer* renderer;
public:
	virtual ~Scene() = default;

	virtual void Start() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void OnGUI(HWND hWND) = 0;
	virtual void OnRender(URenderer* renderer) = 0;
	virtual void Shutdown() = 0;

	void SetRenderer(URenderer* Renderer)
	{
		renderer = Renderer;
	}
};