#include "pch.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Render/UI/Window/Public/UIWindow.h"
#include "Render/UI/ImGui/Public/ImGuiHelper.h"

IMPLEMENT_SINGLETON(UUIManager)

UUIManager::UUIManager()
{
	ImGuiHelper = new UImGuiHelper();
	Initialize();
}

UUIManager::~UUIManager()
{
	if (ImGuiHelper)
	{
		delete ImGuiHelper;
		ImGuiHelper = nullptr;
	}
}

/**
 * @brief UI 매니저 초기화
 */
void UUIManager::Initialize()
{
	if (bIsInitialized)
	{
		return;
	}

	cout << "[UIManager] Initializing UI System..." << endl;

	UIWindows.clear();
	FocusedWindow = nullptr;
	TotalTime = 0.0f;

	bIsInitialized = true;

	cout << "[UIManager] UI system initialized successfully." << endl;
}

/**
 * @brief ImGui를 포함한 UI Manager 초기화
 */
void UUIManager::Initialize(HWND InWindowHandle)
{
	Initialize();

	if (ImGuiHelper)
	{
		ImGuiHelper->Initialize(InWindowHandle);
		cout << "[UIManager] ImGui Initialized Successfully." << endl;
	}
}

/**
 * @brief UI 매니저 종료 및 정리
 */
void UUIManager::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	cout << "[UIManager] Shutting Down UI system..." << endl;

	// ImGui 정리
	if (ImGuiHelper)
	{
		ImGuiHelper->Release();
	}

	// 모든 UI 윈도우 정리
	for (auto* Window : UIWindows)
	{
		if (Window)
		{
			Window->Cleanup();
			delete Window;
		}
	}

	UIWindows.clear();
	FocusedWindow = nullptr;
	bIsInitialized = false;

	cout << "[UIManager] UI System Shut Down Successfully." << endl;
}

/**
 * @brief 모든 UI 윈도우 업데이트
 */
void UUIManager::Update()
{
	if (!bIsInitialized)
	{
		return;
	}

	TotalTime += DT;

	// 모든 UI 윈도우 업데이트
	for (auto* Window : UIWindows)
	{
		if (Window && Window->IsVisible())
		{
			Window->Update();
		}
	}

	// 포커스 상태 업데이트
	UpdateFocusState();
}

/**
 * @brief 모든 UI 윈도우 렌더링
 */
void UUIManager::Render()
{
	if (!bIsInitialized)
	{
		return;
	}

	if (!ImGuiHelper)
	{
		return;
	}

	// ImGui 프레임 시작
	ImGuiHelper->BeginFrame();

	// 우선순위에 따라 정렬 (필요한 경우에만 진행)
	// SortUIWindowsByPriority();

	// 모든 UI 윈도우 렌더링
	for (auto* Window : UIWindows)
	{
		if (Window)
		{
			Window->RenderInternal();
		}
	}

	// ImGui 프레임 종료
	ImGuiHelper->EndFrame();
}

/**
 * @brief UI 윈도우 등록
 * @param InWindow 등록할 UI 윈도우
 * @return 등록 성공 여부
 */
bool UUIManager::RegisterUIWindow(UUIWindow* InWindow)
{
	if (!InWindow)
	{
		cout << "[UIManager] Error: Attempted To Register Null Window!" << endl;
		return false;
	}

	// 이미 등록된 윈도우인지 확인
	auto Iter = std::find(UIWindows.begin(), UIWindows.end(), InWindow);
	if (Iter != UIWindows.end())
	{
		cout << "[UIManager] Warning: Window Already Registered: " << InWindow->GetWindowID() << endl;
		return false;
	}

	// 윈도우 초기화
	try
	{
		InWindow->Initialize();
	}
	catch (const std::exception& e)
	{
		cout << "[UIManager] Error: Failed To Initialize Window " << InWindow->GetWindowID() << ": " << e.what() <<
			endl;
		return false;
	}

	UIWindows.push_back(InWindow);

	cout << "[UIManager] Registered UI Window: " << InWindow->GetWindowID() << " (" << InWindow->GetWindowTitle() << ")"
		<< endl;
	cout << "[UIManager] Total Registered Windows: " << UIWindows.size() << endl;

	return true;
}

/**
 * @brief UI 윈도우 등록 해제
 * @param InWindow 해제할 UI 윈도우
 * @return 해제 성공 여부
 */
bool UUIManager::UnregisterUIWindow(UUIWindow* InWindow)
{
	if (!InWindow)
	{
		return false;
	}

	auto It = std::find(UIWindows.begin(), UIWindows.end(), InWindow);
	if (It == UIWindows.end())
	{
		cout << "[UIManager] Warning: Attempted to unregister non-existent window: " << InWindow->GetWindowID() << endl;
		return false;
	}

	// 포커스된 윈도우였다면 포커스 해제
	if (FocusedWindow == InWindow)
	{
		FocusedWindow = nullptr;
	}

	// 윈도우 정리
	InWindow->Cleanup();

	UIWindows.erase(It);

	cout << "[UIManager] Unregistered UI window: " << InWindow->GetWindowID() << endl;
	cout << "[UIManager] Total registered windows: " << UIWindows.size() << endl;

	return true;
}

/**
 * @brief 이름으로 UI 윈도우 검색
 * @param InWindowName 검색할 윈도우 제목
 * @return 찾은 윈도우 (없으면 nullptr)
 */
UUIWindow* UUIManager::FindUIWindow(const FString& InWindowName) const
{
	for (auto* Window : UIWindows)
	{
		if (Window && Window->GetWindowTitle() == InWindowName)
		{
			return Window;
		}
	}
	return nullptr;
}

/**
 * @brief 모든 UI 윈도우 숨기기
 */
void UUIManager::HideAllWindows() const
{
	for (auto* Window : UIWindows)
	{
		if (Window)
		{
			Window->SetWindowState(EUIWindowState::Hidden);
		}
	}
	cout << "[UIManager] All windows hidden." << endl;
}

/**
 * @brief 모든 UI 윈도우 보이기
 */
void UUIManager::ShowAllWindows() const
{
	for (auto* Window : UIWindows)
	{
		if (Window)
		{
			Window->SetWindowState(EUIWindowState::Visible);
		}
	}
	cout << "[UIManager] All windows shown." << endl;
}

/**
 * @brief 특정 윈도우에 포커스 설정
 */
void UUIManager::SetFocusedWindow(UUIWindow* InWindow)
{
	if (FocusedWindow != InWindow)
	{
		if (FocusedWindow)
		{
			FocusedWindow->OnFocusLost();
		}

		FocusedWindow = InWindow;

		if (FocusedWindow)
		{
			FocusedWindow->OnFocusGained();
		}
	}
}

/**
 * @brief 디버그 정보 출력
 * 필요한 지점에서 호출해서 로그로 체크하는 용도
 */
void UUIManager::PrintDebugInfo() const
{
	cout << "\n=== UI Manager Debug Info ===" << endl;
	cout << "Initialized: " << (bIsInitialized ? "Yes" : "No") << endl;
	cout << "Total Time: " << TotalTime << "s" << endl;
	cout << "Registered Windows: " << UIWindows.size() << endl;
	cout << "Focused Window: " << (FocusedWindow ? FocusedWindow->GetWindowID() : "None") << endl;

	cout << "\n--- Window List ---" << endl;
	for (size_t i = 0; i < UIWindows.size(); ++i)
	{
		auto* Window = UIWindows[i];
		if (Window)
		{
			cout << "[" << i << "] " << Window->GetWindowID() << " (" << Window->GetWindowTitle() << ")" << endl;
			cout << "    State: " << (Window->IsVisible() ? "Visible" : "Hidden") << endl;
			cout << "    Priority: " << Window->GetPriority() << endl;
			cout << "    Focused: " << (Window->IsFocused() ? "Yes" : "No") << endl;
		}
	}
	cout << "===========================\n" << endl;
}

/**
 * @brief UI 윈도우들을 우선순위에 따라 정렬
 */
void UUIManager::SortUIWindowsByPriority()
{
	// 우선순위가 낮을수록 먼저 렌더링되고 가려짐
	std::sort(UIWindows.begin(), UIWindows.end(), [](const UUIWindow* A, const UUIWindow* B)
	{
		if (!A) return false;
		if (!B) return true;
		return A->GetPriority() < B->GetPriority();
	});
}

/**
 * @brief 포커스 상태 업데이트
 */
void UUIManager::UpdateFocusState()
{
	// ImGui에서 현재 포커스된 윈도우 찾기
	UUIWindow* NewFocusedWindow = nullptr;

	for (auto* Window : UIWindows)
	{
		if (Window && Window->IsVisible() && Window->IsFocused())
		{
			NewFocusedWindow = Window;
			break;
		}
	}

	// 포커스 변경시 처리
	if (FocusedWindow != NewFocusedWindow)
	{
		SetFocusedWindow(NewFocusedWindow);
	}
}

/**
 * @brief 윈도우 프로시저 핸들러
 */
LRESULT UUIManager::WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return UImGuiHelper::WndProcHandler(hwnd, msg, wParam, lParam);
}
