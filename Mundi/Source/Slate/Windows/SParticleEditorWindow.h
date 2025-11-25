#pragma once
#include "SWindow.h"
#include "ParticleEditorSections.h"

class FViewport;
class FViewportClient;
class UWorld;
struct ID3D11Device;
class ParticleEditorState;

class SParticleEditorWindow : public SWindow
{
public:
    SParticleEditorWindow();
    virtual ~SParticleEditorWindow();

    bool Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice);

    // SWindow overrides
    virtual void OnRender() override;
    virtual void OnUpdate(float DeltaSeconds) override;
    virtual void OnMouseMove(FVector2D MousePos) override;
    virtual void OnMouseDown(FVector2D MousePos, uint32 Button) override;
    virtual void OnMouseUp(FVector2D MousePos, uint32 Button) override;

    void OnRenderViewport();
    void CreateNewTab();
    void RequestColorPickerFocus();

    bool IsOpen() const { return bIsOpen; }
    void Close() { bIsOpen = false; }

    // Accessors (active tab)
    FViewport* GetViewport() const;
    FViewportClient* GetViewportClient() const;
    void SetColorPickerSpawnPosition(const FVector2D& ScreenPos);
    ParticleEditorState* GetActiveState() const { return ActiveState; }

private:
    // Tabs
    void OpenNewTab(const char* Name = "Particle Editor");
    void CloseTab(int Index);

    // 파티클 에디터 윈도우 UI에는 크게 6종류 파트로 나뉨

    // 뷰포트 내부 헬퍼 함수
    bool DrawSplitter(const char* Id, bool bVertical, float Length,
        float& SizeA, float& SizeB, float MinSizeA, float MinSizeB);

    // 파티클 탭 관련 변수
    ParticleEditorState* ActiveState = nullptr;
    TArray<ParticleEditorState*> Tabs;
    int ActiveTabIndex = -1;

    // 스플리터 관련 변수
    static constexpr float SplitterThickness = 4.0f;
    static constexpr float CollapsedPanelEpsilon = 0.001f;
    static constexpr float MinColumnWidth = 0.0f;
    static constexpr float MinRowHeight = 0.0f;
    float VerticalSplitRatio = 0.5f;
    float LeftColumnSplitRatio = 0.6f;
    float RightColumnSplitRatio = 0.6f;

    // For legacy single-state flows; removed once tabs are stable
    UWorld* World = nullptr;
    ID3D11Device* Device = nullptr;

    // Cached panel regions
    FRect ViewportRect;      // Left-top
    FRect DetailRect;        // Left-bottom
    FRect EmitterRect;       // Right-top
    FRect CurveEditorRect;   // Right-bottom

    // Whether we've applied the initial ImGui window placement
    bool bInitialPlacementDone = false;

    // Request focus on first open
    bool bRequestFocus = false;

    // Window open state
    bool bIsOpen = true;

    // Color picker state
    bool bShowColorPicker = false;
    bool bColorPickerFocusRequested = false;
    bool bColorPickerSpawnPosValid = false;
    FVector2D ColorPickerSpawnPos = FVector2D::Zero();

    // Section widgets
    FParticleEditorToolBarSection ToolBarSection;
    FParticleEditorViewportSection ViewportSection;
    FParticleEditorEmitterSection EmitterSection;
    FParticleEditorDetailSection DetailSection;
    FParticleEditorCurveSection CurveSection;
};
