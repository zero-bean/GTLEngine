#include "pch.h"
#include "Render/UI/Window/Public/UIWindow.h"

int UUIWindow::IssuedWindowID = 0;

UUIWindow::UUIWindow(const FUIWindowConfig& InConfig)
	: Config(InConfig)
	  , CurrentState(InConfig.InitialState)
{
	// 고유한 윈도우 ID 생성
	WindowID = ++IssuedWindowID;

	// 윈도우 플래그 업데이트
	Config.UpdateWindowFlags();

	// 초기 상태 설정
	LastWindowSize = Config.DefaultSize;
	LastWindowPosition = Config.DefaultPosition;

	cout << "[UIWindow] Created: " << WindowID << " (" << Config.WindowTitle << ")" << "\n";
}

/**
 * @brief 내부적으로 사용되는 윈도우 렌더링 처리
 * 서브클래스에서 직접 호출할 수 없도록 접근한정자 설정
 */
void UUIWindow::RenderInternal()
{
	// 숨겨진 상태면 렌더링하지 않음
	if (!IsVisible())
	{
		return;
	}

	// 도킹 설정 적용
	ApplyDockingSettings();

	// ImGui 윈도우 시작
	ImGui::SetNextWindowSize(Config.DefaultSize, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(Config.DefaultPosition, ImGuiCond_FirstUseEver);

	// 크기 제한 설정
	ImGui::SetNextWindowSizeConstraints(Config.MinSize, Config.MaxSize);

	bool bIsOpen = bIsWindowOpen;

	if (ImGui::Begin(Config.WindowTitle.c_str(), &bIsOpen, Config.WindowFlags))
	{
		// 실제 UI 컨텐츠 렌더링 (서브클래스에서 구현)
		Render();

		// 윈도우 정보 업데이트
		UpdateWindowInfo();
	}

	ImGui::End();

	// 윈도우가 닫혔는지 확인
	if (!bIsOpen && bIsWindowOpen)
	{
		if (OnWindowClose())
		{
			bIsWindowOpen = false;
			SetWindowState(EUIWindowState::Hidden);
		}
		else
		{
			bIsWindowOpen = true; // 닫기 취소
		}
	}

	bIsWindowOpen = bIsOpen;
}

/**
 * @brief 윈도우 위치와 크기 자동 설정
 */
void UUIWindow::ApplyDockingSettings() const
{
	ImGuiIO& IO = ImGui::GetIO();
	float ScreenWidth = IO.DisplaySize.x;
	float ScreenHeight = IO.DisplaySize.y;

	switch (Config.DockDirection)
	{
	case EUIDockDirection::Left:
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(Config.DefaultSize.x, ScreenHeight), ImGuiCond_Always);
		break;

	case EUIDockDirection::Right:
		ImGui::SetNextWindowPos(ImVec2(ScreenWidth - Config.DefaultSize.x, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(Config.DefaultSize.x, ScreenHeight), ImGuiCond_Always);
		break;

	case EUIDockDirection::Top:
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(ScreenWidth, Config.DefaultSize.y), ImGuiCond_Always);
		break;

	case EUIDockDirection::Bottom:
		ImGui::SetNextWindowPos(ImVec2(0, ScreenHeight - Config.DefaultSize.y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(ScreenWidth, Config.DefaultSize.y), ImGuiCond_Always);
		break;

	case EUIDockDirection::Center:
		{
			ImVec2 Center = ImVec2(ScreenWidth * 0.5f, ScreenHeight * 0.5f);
			ImVec2 WindowPosition = ImVec2(Center.x - Config.DefaultSize.x * 0.5f,
			                               Center.y - Config.DefaultSize.y * 0.5f);
			ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_Always);
		}
		break;

	case EUIDockDirection::None:
	default:
		// 기본 위치 사용
		break;
	}
}

/**
 * @brief ImGui 컨텍스트에서 현재 윈도우 정보 업데이트
 */
void UUIWindow::UpdateWindowInfo()
{
	// 현재 윈도우가 포커스되어 있는지 확인
	bool bCurrentlyFocused = ImGui::IsWindowFocused();

	if (bCurrentlyFocused != bIsFocused)
	{
		bIsFocused = bCurrentlyFocused;

		if (bIsFocused)
		{
			OnFocusGained();
		}
		else
		{
			OnFocusLost();
		}
	}

	// 윈도우 크기와 위치 업데이트
	LastWindowSize = ImGui::GetWindowSize();
	LastWindowPosition = ImGui::GetWindowPos();
}
