#include "pch.h"
#include "Render/UI/Window/Public/ExperimentalFeatureWindow.h"

#include "Manager/Input/Public/InputManager.h"

constexpr int MaxKeyHistory = 10;

/**
 * @brief 생성자
 */
UExperimentalFeatureWindow::UExperimentalFeatureWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Key Input Status";
	Config.DefaultSize = ImVec2(300, 400);
	Config.DefaultPosition = ImVec2(10, 220);
	Config.MinSize = ImVec2(250, 200);
	Config.DockDirection = EUIDockDirection::Right;
	Config.Priority = 5;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);
}

/**
 * @brief 초기화
 */
void UExperimentalFeatureWindow::Initialize()
{
	RecentKeyPresses.clear();
	KeyPressCount.clear();
	LastMousePosition = FVector(0, 0, 0);
	MouseDelta = FVector(0, 0, 0);

	UE_LOG("ExperimentalFeatureWindow: Successfully Initialized");
}

// /**
//  * @brief 업데이트
//  */
// void UInputStatusWindow::Update()
// {
// 	auto& InputManager = UInputManager::GetInstance();
//
// 	// 마우스 위치 업데이트
// 	FVector CurrentMousePosition = InputManager.GetMousePosition();
// 	MouseDelta = CurrentMousePosition - LastMousePosition;
// 	LastMousePosition = CurrentMousePosition;
//
// 	// 새로운 키 입력 확인 및 히스토리에 추가
// 	TArray<EKeyInput> PressedKeys = InputManager.GetPressedKeys();
// 	for (const auto& Key : PressedKeys)
// 	{
// 		const wchar_t* KeyString = UInputManager::KeyInputToString(Key);
// 		FString KeyName = FString(reinterpret_cast<const char*>(KeyString));
//
// 		// 키 통계 업데이트
// 		if (KeyPressCount.find(KeyName) == KeyPressCount.end())
// 		{
// 			KeyPressCount[KeyName] = 0;
// 		}
// 		KeyPressCount[KeyName]++;
//
// 		// 히스토리에 추가 (중복 방지)
// 		if (RecentKeyPresses.empty() || RecentKeyPresses.back() != KeyName)
// 		{
// 			AddKeyToHistory(KeyName);
// 		}
// 	}
// }
//
// /**
//  * @brief 렌더링
//  */
// void UInputStatusWindow::Render()
// {
// 	auto& InputManager = UInputManager::GetInstance();
// 	TArray<EKeyInput> PressedKeys = InputManager.GetPressedKeys();
//
// 	// 탭으로 구분
// 	if (ImGui::BeginTabBar("InputTabs"))
// 	{
// 		// 현재 입력 탭
// 		if (ImGui::BeginTabItem("Current Input"))
// 		{
// 			RenderKeyList(PressedKeys);
// 			ImGui::EndTabItem();
// 		}
//
// 		// 마우스 정보 탭
// 		if (ImGui::BeginTabItem("Mouse Info"))
// 		{
// 			RenderMouseInfo();
// 			ImGui::EndTabItem();
// 		}
//
// 		// 키 히스토리 탭
// 		if (ImGui::BeginTabItem("Key History"))
// 		{
// 			ImGui::Text("Recent Key Presses:");
// 			ImGui::Separator();
//
// 			for (int i = static_cast<int>(RecentKeyPresses.size()) - 1; i >= 0; --i)
// 			{
// 				ImGui::Text("- %s", RecentKeyPresses[i].c_str());
// 			}
//
// 			if (ImGui::Button("Clear History"))
// 			{
// 				RecentKeyPresses.clear();
// 			}
//
// 			ImGui::EndTabItem();
// 		}
//
// 		// 통계 탭
// 		if (ImGui::BeginTabItem("Statistics"))
// 		{
// 			RenderKeyStatistics();
// 			ImGui::EndTabItem();
// 		}
//
// 		// 도움말 탭
// 		if (ImGui::BeginTabItem("Help"))
// 		{
// 			RenderControlHelp();
// 			ImGui::EndTabItem();
// 		}
//
// 		ImGui::EndTabBar();
// 	}
// }

void UExperimentalFeatureWindow::AddKeyToHistory(const FString& InKeyName)
{
	RecentKeyPresses.push_back(InKeyName);

	// 최대 히스토리 크기 제한
	if (RecentKeyPresses.size() > MaxKeyHistory)
	{
		RecentKeyPresses.erase(RecentKeyPresses.begin());
	}
}

void UExperimentalFeatureWindow::RenderKeyList(const TArray<EKeyInput>& InPressedKeys)
{
	ImGui::Text("Pressed Keys:");
	ImGui::Separator();

	if (InPressedKeys.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(No Input)");
	}
	else
	{
		for (const EKeyInput& Key : InPressedKeys)
		{
			const wchar_t* KeyString = UInputManager::KeyInputToString(Key);
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "- %ls", KeyString);
		}

		ImGui::Separator();
		ImGui::Text("Total Keys: %d", static_cast<int>(InPressedKeys.size()));
	}
}

void UExperimentalFeatureWindow::RenderMouseInfo() const
{
	ImGui::Text("Mouse Position: (%.0f, %.0f)", LastMousePosition.X, LastMousePosition.Y);
	ImGui::Text("Mouse Delta: (%.2f, %.2f)", MouseDelta.X, MouseDelta.Y);
	ImGui::Text("Mouse Speed: %.2f", MouseDelta.Length());

	// 마우스 위치를 작은 그래프로 표시
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
	ImVec2 CanvasSize = ImVec2(200, 100);

	// 배경
	DrawList->AddRectFilled(CanvasPos, ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
	                        IM_COL32(50, 50, 50, 255));

	// 마우스 위치 점
	ImVec2 MousePosNormalized = ImVec2(
		(LastMousePosition.X / 1920.0f) * CanvasSize.x,
		(LastMousePosition.Y / 1080.0f) * CanvasSize.y
	);

	DrawList->AddCircleFilled(ImVec2(CanvasPos.x + MousePosNormalized.x, CanvasPos.y + MousePosNormalized.y),
	                          3.0f, IM_COL32(255, 0, 0, 255));

	ImGui::Dummy(CanvasSize);
}

void UExperimentalFeatureWindow::RenderControlHelp()
{
	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Game Controls:");
	ImGui::Separator();
	ImGui::Text("ESC: Exit");
	ImGui::Text("Space: Action (Hold/Release)");
	ImGui::Text("A/D: Move Left/Right");
	ImGui::Text("WASD: Movement");
	ImGui::Text("Mouse: Look Around");
	ImGui::Text("F1-F4: Function Keys");

	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "UI Controls:");
	ImGui::Text("Click & Drag: Move Windows");
}

void UExperimentalFeatureWindow::RenderKeyStatistics()
{
	ImGui::Text("Key Press Statistics:");
	ImGui::Separator();

	if (KeyPressCount.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No statistics yet");
	}
	else
	{
		// 통계를 카운트 순으로 정렬
		TArray<std::pair<FString, int>> SortedStats;
		for (const auto& Pair : KeyPressCount)
		{
			SortedStats.push_back(Pair);
		}

		std::sort(SortedStats.begin(), SortedStats.end(),
		          [](const auto& A, const auto& B) { return A.second > B.second; });

		for (const auto& [Key, Count] : SortedStats)
		{
			ImGui::Text("%s: %d times", Key.c_str(), Count);
		}

		if (ImGui::Button("Clear Statistics"))
		{
			KeyPressCount.clear();
		}
	}
}
