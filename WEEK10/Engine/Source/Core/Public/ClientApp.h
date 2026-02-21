#pragma once
#include "Global/BuildConfig.h"

class FAppWindow;

#if !WITH_EDITOR
#include "Subsystem/Public/SubsystemCollection.h"
class UGameInstance;
#endif

/**
 * @brief Main Client Class
 * Application Entry Point, Manage Overall Execution Flow
 *
 * @var AcceleratorTable 키보드 단축키 테이블 핸들
 * @var MainMessage 윈도우 메시지 구조체
 * @var Window 윈도우 객체 포인터
 */
class FClientApp
{
public:
    int Run(HINSTANCE InInstanceHandle, int InCmdShow);

#if !WITH_EDITOR
    void SetScenePath(const char* InScenePath) { ScenePath = InScenePath; }
#endif

    // Special Member Function
    FClientApp();
    ~FClientApp();

private:
    int InitializeSystem() const;
    void UpdateSystem() const;
    void MainLoop();
	void ShutdownSystem() const;

    HACCEL AcceleratorTable;
    MSG MainMessage;
    FAppWindow* Window;

#if !WITH_EDITOR
    // StandAlone Mode: Subsystem Collection으로 관리
    mutable FEngineSubsystemCollection EngineSubsystems;
    const char* ScenePath = nullptr;
#endif
};
