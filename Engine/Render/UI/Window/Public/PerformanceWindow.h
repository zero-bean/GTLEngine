#pragma once
#include "UIWindow.h"

/**
 * @brief 프레임 성능 정보를 표시하는 UI 윈도우
 * 기존에 있던 Frame Performance Info를 분리한 클래스
 */
class UPerformanceWindow : public UUIWindow
{
public:
	UPerformanceWindow();
	void Initialize() override;
	void Render() override;
	void Update() override;

private:
	// 성능 통계
	float FrameTimeHistory[60] = {0};
	int FrameTimeIndex = 0;
	float AverageFrameTime = 0.0f;

	// FPS 관련
	float CurrentFPS = 0.0f;
	float MinFPS = 999.0f;
	float MaxFPS = 0.0f;

	// 시간 관련
	float TotalGameTime = 0.0f;
	float CurrentDeltaTime = 0.0f;

	// UI 설정
	bool bShowDetailedStats = true;
	bool bShowGraph = true;

	// FPS 색상 계산
	static ImVec4 GetFPSColor(float InFPS);
};


