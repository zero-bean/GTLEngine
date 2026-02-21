#pragma once
#include"Engine.h"

class UGameEngine;
class UEditorEngine :
    public UEngine
{
public:
    DECLARE_CLASS(UEditorEngine, UEngine)
    UEditorEngine();
    UGameEngine* GameEngine = nullptr; // PIE 실행용
    
    virtual void Tick(float DeltaSeconds) override;
    virtual void Render() override;
    void StartPIE();
    void EndPIE();
    bool GetPIEShutdownRequested() { return bPIEShutdownRequested; };

    // 에디터 전용 단축키 처리 (Ctrl+C/V 등)
    void ProcessEditorShortcuts();

    // Alt+드래그 복제 처리
    void ProcessAltDragDuplication();
protected:
    ~UEditorEngine();
private:
    // 지연 삭제용
    UGameEngine* PendingDeleteGameEngine = nullptr;
    UWorld* PendingDeletePIEWorld = nullptr;
    bool bPIEShutdownRequested = false;

    // Alt+드래그 복제 상태
    bool bAltDragDuplicationHandled = false;

};

