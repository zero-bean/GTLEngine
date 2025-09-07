#include "pch.h"
#include "Render/UI/Window/Public/PerformanceWindow.h"

#include "Manager/Time/Public/TimeManager.h"

/**
 * @brief 생성자
 */
UPerformanceWindow::UPerformanceWindow()
{
    // Performance 윈도우 설정
    FUIWindowConfig Config;
    Config.WindowTitle = "Frame Performance Info";
    Config.DefaultSize = ImVec2(300, 200);
    Config.DefaultPosition = ImVec2(10, 10);
    Config.MinSize = ImVec2(250, 150);
    Config.DockDirection = EUIDockDirection::Right;
    Config.Priority = 10; // 높은 우선순위
    Config.bResizable = true;
    Config.bMovable = true;
    Config.bCollapsible = true;

    Config.UpdateWindowFlags();
    SetConfig(Config);
}

/**
 * @brief 초기화
 */
void UPerformanceWindow::Initialize()
{
    cout << "[PerformanceWindow] Initialized" << endl;

    // 히스토리 초기화
    for (int i = 0; i < 60; ++i)
    {
        FrameTimeHistory[i] = 0.0f;
    }

    FrameTimeIndex = 0;
    AverageFrameTime = 0.0f;
    CurrentFPS = 0.0f;
    MinFPS = 999.0f;
    MaxFPS = 0.0f;
    TotalGameTime = 0.0f;
}

/**
 * @brief 업데이트
 */
void UPerformanceWindow::Update()
{
    CurrentDeltaTime = DT;
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

/**
 * @brief 렌더링
 */
void UPerformanceWindow::Render()
{
    auto& TimeManager = UTimeManager::GetInstance();

    // FPS 표시 (색상 포함)
    ImVec4 FPSColor = GetFPSColor(CurrentFPS);
    ImGui::TextColored(FPSColor, "FPS: %.1f", CurrentFPS);

    // Delta Time 정보
    ImGui::Text("Frame Time: %.2f ms", CurrentDeltaTime * 1000.0f);
    ImGui::Text("Game Time: %.1f s", TotalGameTime);

    ImGui::Separator();

    // 게임 일시정지 상태
    if (TimeManager.IsPaused())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Game Paused");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Game Running");
    }

    // 상세 통계 토글
    if (ImGui::Checkbox("Show Details", &bShowDetailedStats))
    {
        // 창 크기 조정
        if (bShowDetailedStats)
        {
            GetMutableConfig().DefaultSize = ImVec2(300, 350);
        }
        else
        {
            GetMutableConfig().DefaultSize = ImVec2(300, 200);
        }
    }

    // 상세 정보 표시
    if (bShowDetailedStats)
    {
        ImGui::Separator();
        ImGui::Text("Statistics:");
        ImGui::Text("  Min FPS: %.1f", MinFPS);
        ImGui::Text("  Max FPS: %.1f", MaxFPS);
        ImGui::Text("  Avg Frame Time: %.2f ms", AverageFrameTime);

        // 그래프 토글
        ImGui::Checkbox("Show Graph", &bShowGraph);

        if (bShowGraph)
        {
            ImGui::Separator();
            ImGui::Text("Frame Time History:");
            ImGui::PlotLines("##FrameTime", FrameTimeHistory, 60, FrameTimeIndex,
                           ("Avg: " + to_string(AverageFrameTime) + " ms").c_str(),
                           0.0f, 50.0f, ImVec2(0, 80));
        }

        if (ImGui::Button("Reset Stats"))
        {
            MinFPS = 999.0f;
            MaxFPS = 0.0f;
            for (int i = 0; i < 60; ++i)
            {
                FrameTimeHistory[i] = 0.0f;
            }
        }
    }
}

ImVec4 UPerformanceWindow::GetFPSColor(float InFPS)
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
