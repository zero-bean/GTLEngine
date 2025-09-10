#pragma once
#include "Widget.h"

class UFPSWidget :
	public UWidget
{
public:
	UFPSWidget();

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	static ImVec4 GetFPSColor(float InFPS);

private:
	float FrameTimeHistory[60] = {};
	int FrameTimeIndex = 0;
	float AverageFrameTime = 0.0f;

	float CurrentFPS = 0.0f;
	float MinFPS = 999.0f;
	float MaxFPS = 0.0f;

	float TotalGameTime = 0.0f;
	float CurrentDeltaTime = 0.0f;
};
