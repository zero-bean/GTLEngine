#include "pch.h"
#include "Core/Public/ClientApp.h"

#include "Core/Public/AppWindow.h"
#include "Manager/Input/Public/InputManager.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Time/Public/TimeManager.h"

#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/SceneView.h"
#include "Render/UI/Viewport/Public/GameViewportClient.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Utility/Public/ScopeCycleCounter.h"

#if WITH_EDITOR
#include "Editor/Public/Editor.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#else
#include "Level/Public/GameInstance.h"
#endif

#ifdef IS_OBJ_VIEWER
#include "Utility/Public/FileDialog.h"
#endif

#ifdef _DEBUG
#include "Global/Memory.h"
#endif

FClientApp::FClientApp()
{
#ifdef _DEBUG
	InitializeMemoryTracking();
#endif
}

FClientApp::~FClientApp()
{
#ifdef _DEBUG
	ShutdownMemoryTracking();
#endif
}
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
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(0);
#endif

	// Window Object Initialize
	Window = new FAppWindow(this);
	if (!Window->Init(InInstanceHandle, InCmdShow))
	{
		assert(!"Window Creation Failed");
		return 0;
	}

	// Create Console
	// #ifdef _DEBUG
	// 	Window->InitializeConsole();
	// #endif

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
	ShutdownSystem();

	return static_cast<int>(MainMessage.wParam);
}

/**
 * @brief Initialize System For Game Execution
 */
int FClientApp::InitializeSystem() const
{
	// 현재 시간을 랜덤 시드로 설정
	srand(static_cast<unsigned int>(time(NULL)));

	// Initialize By Get Instance
	UTimeManager::GetInstance();
	UInputManager::GetInstance();

	auto& Renderer = URenderer::GetInstance();
	Renderer.Init(Window->GetWindowHandle());

	UAssetManager::GetInstance().Initialize();

#if WITH_EDITOR
	// StatOverlay Initialize
	auto& StatOverlay = UStatOverlay::GetInstance();
	StatOverlay.Initialize();

	// ViewportManager Initialize - FutureEngine: Initialize()를 먼저 호출해야 함!
	auto& ViewportManager = UViewportManager::GetInstance();
	ViewportManager.Initialize(Window);  // 중요: Viewports/Clients 벡터 초기화

	GEditor = NewObject<UEditorEngine>();  // UEditor 생성자가 ViewportManager::GetClients()를 사용

	// UIManager Initialize
	auto& UIManager = UUIManager::GetInstance();
	UIManager.Initialize(Window->GetWindowHandle());
	UUIWindowFactory::CreateDefaultUILayout();
#else
	// StandAlone Mode: Initialize GameInstance Subsystem
	EngineSubsystems.RegisterSubsystemClass<UGameInstance>();
	EngineSubsystems.Initialize(nullptr);

	// GameInstance World 초기화
	UGameInstance* GameInstance = EngineSubsystems.GetSubsystem<UGameInstance>();
	if (GameInstance)
	{
		GameInstance->InitializeWorld(Window, ScenePath);
	}
	else
	{
		UE_LOG_ERROR("ClientApp: Failed to create GameInstance");
		return E_FAIL;
	}
#endif

	return S_OK;
}

/**
 * @brief Update System While Game Processing
 */
void FClientApp::UpdateSystem() const
{
	auto& TimeManager = UTimeManager::GetInstance();
	auto& InputManager = UInputManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();

	{
		TIME_PROFILE(TimeManager)
		TimeManager.Update();
	}
	{
		TIME_PROFILE(InputManager)
		InputManager.Update(Window);
	}

#if WITH_EDITOR
	{
		TIME_PROFILE(ViewportManager)
		UViewportManager::GetInstance().Update();
	}
	{
		TIME_PROFILE(GEditor)
		GEditor->Tick(DT);
	}
	{
		TIME_PROFILE(UIManager)
		UUIManager::GetInstance().Update();
	}
	{
		TIME_PROFILE(Renderer)
		Renderer.Update();
	}
#else
	// StandAlone Mode: GameInstance Tick
	{
		TIME_PROFILE(GameInstance)
		UGameInstance* GameInstance = EngineSubsystems.GetSubsystem<UGameInstance>();
		if (GameInstance)
		{
			GameInstance->Tick(DT);
		}
	}
	{
		TIME_PROFILE(Renderer)
		// StandAlone 렌더링: GameViewportClient->Draw 호출
		UGameInstance* GameInstance = EngineSubsystems.GetSubsystem<UGameInstance>();
		if (GameInstance)
		{
			FViewport* Viewport = GameInstance->GetViewport();
			UGameViewportClient* ViewportClient = GameInstance->GetViewportClient();

			if (Viewport && ViewportClient)
			{
				Renderer.RenderBegin();
				ViewportClient->Draw(Viewport);
				Renderer.RenderEnd();
			}
		}
	}
#endif

	UInputManager::GetInstance().ClearMouseWheelDelta();
}

/**
 * @brief Execute Main Message Loop
 * 윈도우 메시지 처리 및 게임 시스템 업데이트를 담당
 */
void FClientApp::MainLoop()
{
#ifdef IS_OBJ_VIEWER
	if (!OpenObjFromFileDialog())
	{
		MessageBoxW(
			nullptr,
			L"파일을 열 수 없습니다",
			L"오류",
			MB_OK | MB_ICONERROR
		);
		std::exit(1);
	}
#endif

	bool bIsExit = false;
	while (!bIsExit)
	{
		TStatId StatId("DeltaTime");
		FScopeCycleCounter CycleCounter(StatId);
		// Async Message Process
		while (PeekMessage(&MainMessage, nullptr, 0, 0, PM_REMOVE))
		{
			// Process Termination
			if (MainMessage.message == WM_QUIT)
			{
				bIsExit = true;
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
		UpdateSystem();

		UTimeManager::GetInstance().SetDeltaTime(static_cast<float>(CycleCounter.Finish()) / 1000.0f);
	}
}

/**
 * @brief 시스템 종료 처리
 * 모든 리소스를 안전하게 해제하고 매니저들을 정리합니다.
 */
void FClientApp::ShutdownSystem() const
{
#if WITH_EDITOR
	UStatOverlay::GetInstance().Release();
	UUIManager::GetInstance().Shutdown();
	delete GEditor;
#else
	// StandAlone Mode: Deinitialize Subsystems
	EngineSubsystems.Deinitialize();
#endif

	delete Window;
	UAssetManager::GetInstance().Release();
	URenderer::GetInstance().Release();
}
