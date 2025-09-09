#include "pch.h"
#include "Core/Public/AppWindow.h"

#include "ImGui/imgui.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Input/Public/InputManager.h"

FAppWindow::FAppWindow(FClientApp* InOwner)
	: Owner(InOwner), InstanceHandle(nullptr), MainWindowHandle(nullptr)
{
}

FAppWindow::~FAppWindow() = default;

bool FAppWindow::Init(HINSTANCE InInstance, int InCmdShow)
{
	InstanceHandle = InInstance;

	WCHAR WindowClass[] = L"UnlearnEngineWindowClass";

	WNDCLASSW wndclass = {0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass};
	RegisterClassW(&wndclass);

	MainWindowHandle = CreateWindowExW(0, WindowClass, L"",
	                                   WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
	                                   CW_USEDEFAULT, CW_USEDEFAULT,
	                                   Render::INIT_SCREEN_WIDTH, Render::INIT_SCREEN_HEIGHT,
	                                   nullptr, nullptr, InInstance, nullptr);

	if (!MainWindowHandle)
	{
		return false;
	}

	ShowWindow(MainWindowHandle, InCmdShow);
	UpdateWindow(MainWindowHandle);
	SetTitle(L"Unlearn Engine Project");

	return true;
}

/**
 * @brief Initialize Console & Redirect IO
 * 현재 ImGui로 기능을 넘기면서 사용은 하지 않으나 코드는 유지
 */
void FAppWindow::InitializeConsole()
{
	// Error Handle
	if (!AllocConsole())
	{
		MessageBoxW(nullptr, L"콘솔 생성 실패", L"Error", 0);
	}

	// Console 출력 지정
	FILE* FilePtr;
	(void)freopen_s(&FilePtr, "CONOUT$", "w", stdout);
	(void)freopen_s(&FilePtr, "CONOUT$", "w", stderr);
	(void)freopen_s(&FilePtr, "CONIN$", "r", stdin);

	// Console Menu Setting
	HWND ConsoleWindow = GetConsoleWindow();
	HMENU MenuHandle = GetSystemMenu(ConsoleWindow, FALSE);
	if (MenuHandle != nullptr)
	{
		EnableMenuItem(MenuHandle, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		DeleteMenu(MenuHandle, SC_CLOSE, MF_BYCOMMAND);
	}
}

FAppWindow* FAppWindow::GetWindowInstance(HWND InWindowHandle, uint32 InMessage, LPARAM InLParam)
{
	if (InMessage == WM_NCCREATE)
	{
		CREATESTRUCT* CreateStruct = reinterpret_cast<CREATESTRUCT*>(InLParam);
		FAppWindow* WindowInstance = static_cast<FAppWindow*>(CreateStruct->lpCreateParams);
		SetWindowLongPtr(InWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(WindowInstance));

		return WindowInstance;
	}

	return reinterpret_cast<FAppWindow*>(GetWindowLongPtr(InWindowHandle, GWLP_USERDATA));
}

LRESULT CALLBACK FAppWindow::WndProc(HWND InWindowHandle, uint32 InMessage, WPARAM InWParam,
                                     LPARAM InLParam)
{
	if (UUIManager::WndProcHandler(InWindowHandle, InMessage, InWParam, InLParam))
	{
		if (ImGui::GetIO().WantCaptureMouse)
		{
			return true;
		}
	}

	UInputManager::GetInstance().ProcessKeyMessage(InMessage, InWParam, InLParam);

	switch (InMessage)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(InWindowHandle, InMessage, InWParam, InLParam);
	}

	return 0;
}

void FAppWindow::SetTitle(const wstring& InNewTitle) const
{
	SetWindowTextW(MainWindowHandle, InNewTitle.c_str());
}
