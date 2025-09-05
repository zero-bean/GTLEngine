#include "pch.h"
#include "Core/Public/ClientApp.h"

#include "Core/Public/AppWindow.h"
#include "Manager/ImGui/Public/ImGuiManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Render/Public/Renderer.h"

FClientApp::FClientApp() = default;

FClientApp::~FClientApp() = default;

/**
 * @brief Client Main Runtime Function
 * App 초기화, Main Loop 실행을 통한 전체 Cycle
 *
 * @param InInstanceHandle Process Instance Handle
 * @param InCmdShow Window Display Method
 *
 *
 * @return Program Termination Code
 */
int FClientApp::Run(HINSTANCE InInstanceHandle, int InCmdShow)
{
	// Memory Leak Detection & Report
	// #ifdef _DEBUG
	// 	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// 	_CrtSetBreakAlloc(0);
	// #endif

	// Window Object Initialize
	Window = new FAppWindow(this);
	if (!Window->Init(InInstanceHandle, InCmdShow))
	{
		assert(!"Window Creation Failed");
		return 0;
	}

	// Create Console
#ifdef _DEBUG
	Window->InitializeConsole();
#endif

	// Keyboard Accelerator Table Setting
	// AcceleratorTable = LoadAccelerators(InInstanceHandle, MAKEINTRESOURCE(IDC_CLIENT));

	// Initialize Base System
	int InitResult = InitializeSystem();
	if (InitResult != S_OK)
	{
		assert(!"Initialize Failed");
		return 0;
	}

	// Execute Main Loop
	MainLoop();

	// Termination Process
	delete Window;

	return static_cast<int>(MainMessage.wParam);
}

/**
 * @brief Initialize System For Game Execution
 */
int FClientApp::InitializeSystem() const
{
	// Initialize By Get Instance
	UTimeManager::GetInstance();
	UInputManager::GetInstance();

	auto& Renderer = URenderer::GetInstance();
	Renderer.Init(Window->GetWindowHandle());

	return S_OK;
}

/**
 * @brief Update System While Game Processing
 */
void FClientApp::UpdateSystem()
{
	auto& TimeManager = UTimeManager::GetInstance();
	auto& InputManager = UInputManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();
	auto& ImGuiManager = UImGuiManager::GetInstance();

	TimeManager.Update();
	InputManager.Update();

	Renderer.Prepare();
	Renderer.PrepareShader();

	// TODO(KHJ): 실제 나머지 렌더링 가져올 것
	ImGuiManager.Render();

	Renderer.SwapBuffer();
}

/**
 * @brief Execute Main Message Loop
 * 윈도우 메시지 처리 및 게임 시스템 업데이트를 담당
 */
void FClientApp::MainLoop()
{
	while (true)
	{
		// Async Message Process
		if (PeekMessage(&MainMessage, nullptr, 0, 0, PM_REMOVE))
		{
			// Process Termination
			if (MainMessage.message == WM_QUIT)
			{
				break;
			}
			// Shortcut Key Processing
			if (!TranslateAccelerator(MainMessage.hwnd, AcceleratorTable, &MainMessage))
			{
				TranslateMessage(&MainMessage);
				DispatchMessage(&MainMessage);
			}
		}
		// Game System Update
		else
		{
			UpdateSystem();
		}
	}
}
