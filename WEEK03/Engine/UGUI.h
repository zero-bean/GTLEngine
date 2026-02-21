#pragma once
#include "stdafx.h"
#include "UEngineSubsystem.h"
#include "UClass.h"

// Forward declaration
class UTimeManager;

class UGUI : public UEngineSubsystem
{
    DECLARE_UCLASS(UGUI, UEngineSubsystem)
private:
    bool bInitialized;
    bool bShowDemoWindow;

public:
    UGUI();
    ~UGUI();

    // Initialization and cleanup
    bool Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext);
    void Shutdown();

    // Frame management
    void BeginFrame();
    void EndFrame();
    void Render();

    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;
};