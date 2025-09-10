#include "pch.h"
#include "Render/UI/Widget/Public/FPSWidget.h"

#include "Manager/Time/Public/TimeManager.h"

UFPSWidget::UFPSWidget()
	: UWidget("FPS Widget")
{
}

void UFPSWidget::Initialize()
{
	// Do Nothing Here
}

void UFPSWidget::Update()
{	CurrentDeltaTime = DT;
	TotalGameTime += DT;

	auto& TimeManager = UTimeManager::GetInstance();
	CurrentFPS = TimeManager.GetFPS();

	// FPS 통계 업데이트
	MaxFPS = max(CurrentFPS, MaxFPS);
	MinFPS = min(CurrentFPS, MinFPS);

	// 프레임 시간 히스토리 업데이트
	FrameTimeHistory[FrameTimeIndex] = DT * 1000.0f;
	FrameTimeIndex = (FrameTimeIndex + 1) % 60;

	// 평균 프레임 시간 계산
	float Total = 0.0f;
	for (int i = 0; i < 60; ++i)
	{
		Total += FrameTimeHistory[i];
	}
	AverageFrameTime = Total / 60.0f;
}


void UFPSWidget::RenderWidget()
{
	// FPS 표시 (색상 포함)
	ImVec4 FPSColor = GetFPSColor(CurrentFPS);
	ImGui::TextColored(FPSColor, "FPS: %.1f", CurrentFPS);

	// Delta Time 정보
	ImGui::Text("Frame Time: %.2f ms", CurrentDeltaTime * 1000.0f);
	ImGui::Text("Game Time: %.1f s", TotalGameTime);

	ImGui::Separator();
}

ImVec4 UFPSWidget::GetFPSColor(float InFPS)
{
	if (InFPS >= 60.0f)
	{
		return {0.0f, 1.0f, 0.0f, 1.0f}; // 녹색 (우수)
	}
	else if (InFPS >= 30.0f)
	{
		return {1.0f, 1.0f, 0.0f, 1.0f}; // 노란색 (보통)
	}
	else
	{
		return {1.0f, 0.0f, 0.0f, 1.0f}; // 빨간색 (주의)
	}
}
