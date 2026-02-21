#pragma once
#include "Manager/Config/Public/ConfigManager.h"

class SWindow;
class SSplitter;
class FAppWindow;
class UCamera;
class FViewport;
class FViewportClient;
class UConfigManager;

UCLASS()
class UViewportManager :
    public UObject
{
    GENERATED_BODY()
    DECLARE_SINGLETON_CLASS(UViewportManager, UObject)

public:
    // ========================================
    // Lifecycle
    // ========================================
    void Initialize(FAppWindow* InWindow);
    void Update();
    void RenderOverlay() const;
    void Release();
    void TickInput();

    // ========================================
    // Viewport Layout & Animation
    // ========================================
    void SetViewportLayout(EViewportLayout InViewportLayout);
    void StartLayoutAnimation(bool bSingleToQuad, int32 ViewportIndex = -1);
    bool IsAnimating() const { return ViewportAnimation.bIsAnimating; }
    EViewportLayout GetViewportLayout() const { return ViewportLayout; }

    // ========================================
    // Viewport Access & Queries
    // ========================================
    SWindow* GetRoot() const { return Root; }
    SWindow* GetQuadRoot() const { return QuadRoot; }
    TArray<FViewport*>& GetViewports() { return Viewports; }
    TArray<FViewportClient*>& GetClients() { return Clients; }

    int32 GetActiveIndex() const { return ActiveIndex; }
    FRect GetActiveViewportRect() const { return ActiveViewportRect; }
    int32 GetMouseHoveredViewportIndex() const;
    void GetLeafRects(TArray<FRect>& OutRects) const;

    EViewModeIndex GetViewportViewMode(int32 Index) const;
    EViewModeIndex GetActiveViewportViewMode() const { return GetViewportViewMode(ActiveIndex); }

    // ========================================
    // Viewport Modification
    // ========================================
    void SetRoot(SWindow* InRoot) { Root = InRoot; }
    void SetActiveIndex(int32 InActiveIndex) { ActiveIndex = InActiveIndex; }

    void SetActiveViewportRect(FRect InActiveViewportRect)
    {
        ActiveViewportRect = InActiveViewportRect;
    }

    void SetViewportViewMode(int32 Index, EViewModeIndex InMode);

    void SetActiveViewportViewMode(EViewModeIndex InMode)
    {
        SetViewportViewMode(ActiveIndex, InMode);
    }

    // ========================================
    // Save/Load & Persistence
    // ========================================
    void SaveViewportLayoutToConfig();
    void LoadViewportLayoutFromConfig();
    void SaveCameraSettingsToConfig();
    void LoadCameraSettingsFromConfig();
    void SerializeViewports(bool bInIsLoading, JSON& InOutHandle);

    // ========================================
    // Camera Settings
    // ========================================
    float GetEditorCameraSpeed() const { return EditorCameraSpeed; }
    void SetEditorCameraSpeed(float InSpeed);

    float GetSharedOrthoZoom() const { return SharedOrthoZoom; }
    void SetSharedOrthoZoom(float InZoom)
    {
        SharedOrthoZoom = InZoom;
        UConfigManager::GetInstance().SetCachedSharedOrthoZoom(InZoom);
    }

    const FVector& GetOrthoGraphicCameraPoint() const { return OrthoGraphicCameraPoint; }
    void SetOrthoGraphicCameraPoint(const FVector& InPoint) { OrthoGraphicCameraPoint = InPoint; }

    const TArray<FVector>& GetInitialOffsets() const { return InitialOffsets; }
    void UpdateInitialOffset(int32 Index, const FVector& NewOffset);

    static constexpr float MIN_CAMERA_SPEED = 1.0f;
    static constexpr float MAX_CAMERA_SPEED = 512.0f;
    static constexpr float DEFAULT_CAMERA_SPEED = 50.0f;

    // 카메라 속도 레벨
    static constexpr int32 CAMERA_SPEED_LEVELS = 8;
    static constexpr float CAMERA_SPEED_TABLE[CAMERA_SPEED_LEVELS] = {
        4.0f,     // Level 1
        16.0f,    // Level 2
        32.0f,    // Level 3
        64.0f,    // Level 4 (기본)
        128.0f,   // Level 5
        256.0f,   // Level 6
        384.0f,   // Level 7
        512.0f    // Level 8
    };

    // ========================================
    // Location Snap Settings
    // ========================================
    bool IsLocationSnapEnabled() const { return bLocationSnapEnabled; }
    void SetLocationSnapEnabled(bool bEnabled)
    {
        bLocationSnapEnabled = bEnabled;
        UConfigManager::GetInstance().SetCachedLocationSnapEnabled(bEnabled);
    }
    float GetLocationSnapValue() const { return LocationSnapValue; }
    void SetLocationSnapValue(float InValue)
    {
        LocationSnapValue = InValue;
        UConfigManager::GetInstance().SetCachedLocationSnapValue(InValue);
        UConfigManager::GetInstance().SaveCellSize(InValue);
    }
    static constexpr float DEFAULT_LOCATION_SNAP_VALUE = 10.0f;

    // ========================================
    // Rotation Snap Settings
    // ========================================
    bool IsRotationSnapEnabled() const { return bRotationSnapEnabled; }
    void SetRotationSnapEnabled(bool bEnabled)
    {
        bRotationSnapEnabled = bEnabled;
        UConfigManager::GetInstance().SetCachedRotationSnapEnabled(bEnabled);
    }
    float GetRotationSnapAngle() const { return RotationSnapAngle; }
    void SetRotationSnapAngle(float InAngle)
    {
        RotationSnapAngle = InAngle;
        UConfigManager::GetInstance().SetCachedRotationSnapAngle(InAngle);
    }
    static constexpr float DEFAULT_ROTATION_SNAP_ANGLE = 10.0f;

    // ========================================
    // Scale Snap Settings
    // ========================================
    bool IsScaleSnapEnabled() const { return bScaleSnapEnabled; }
    void SetScaleSnapEnabled(bool bEnabled)
    {
        bScaleSnapEnabled = bEnabled;
        UConfigManager::GetInstance().SetCachedScaleSnapEnabled(bEnabled);
    }
    float GetScaleSnapValue() const { return ScaleSnapValue; }
    void SetScaleSnapValue(float InValue)
    {
        ScaleSnapValue = InValue;
        UConfigManager::GetInstance().SetCachedScaleSnapValue(InValue);
    }
    static constexpr float DEFAULT_SCALE_SNAP_VALUE = 1.0f;

    // ========================================
    // Splitter Management
    // ========================================
    void PersistSplitterRatios();
    void UpdateIniSaveSharedRatios();
    bool IsAnySplitterDragging() const;

    // ========================================
    // PIE & Interaction
    // ========================================
    int32 GetPIEActiveViewportIndex() const { return PIEActiveViewportIndex; }
    void SetPIEActiveViewportIndex(int32 InIndex) { PIEActiveViewportIndex = InIndex; }

    int32 GetLastClickedViewportIndex() const { return LastClickedViewportIndex; }
    void SetLastClickedViewportIndex(int32 InIndex) { LastClickedViewportIndex = InIndex; }

private:
    // ========================================
    // Window Tree State
    // ========================================
    SWindow* Root = nullptr;
    SWindow* QuadRoot = nullptr;
    SWindow* Leaves[4] = {nullptr, nullptr, nullptr, nullptr};
    SWindow* Capture = nullptr;

    // ========================================
    // Viewport State
    // ========================================
    TArray<FViewport*> Viewports;
    TArray<FViewportClient*> Clients;
    FRect ActiveViewportRect;

    // ========================================
    // Layout State
    // ========================================
    EViewportLayout ViewportLayout = EViewportLayout::Single;
    int32 ActiveIndex = 2;
    int32 LastClickedViewportIndex = 0;
    int32 ActiveRmbViewportIdx = -1;

    // ========================================
    // Splitter State
    // ========================================
    SSplitter* SplitterV = nullptr;
    SSplitter* LeftSplitterH = nullptr;
    SSplitter* RightSplitterH = nullptr;

    float IniSaveSharedV = 0.5f;
    float IniSaveSharedH = 0.5f;
    float SplitterValueV = 0.5f;
    float SplitterValueH = 0.5f;

    // ========================================
    // SWindow Pointers (Quad Layout)
    // ========================================
    SWindow* LeftTopWindow = nullptr;
    SWindow* LeftBottomWindow = nullptr;
    SWindow* RightTopWindow = nullptr;
    SWindow* RightBottomWindow = nullptr;

    // ========================================
    // Camera State
    // ========================================
    TArray<FVector> InitialOffsets;
    FVector OrthoGraphicCameraPoint{0.0f, 0.0f, 0.0f};
    float SharedOrthoZoom = 500.0f;
    float EditorCameraSpeed = DEFAULT_CAMERA_SPEED;

    // ========================================
    // PIE State
    // ========================================
    int32 PIEActiveViewportIndex = -1;

    // ========================================
    // Location Snap State
    // ========================================
    bool bLocationSnapEnabled = false;
    float LocationSnapValue = DEFAULT_LOCATION_SNAP_VALUE;

    // ========================================
    // Rotation Snap State
    // ========================================
    bool bRotationSnapEnabled = true;
    float RotationSnapAngle = DEFAULT_ROTATION_SNAP_ANGLE;

    // ========================================
    // Scale Snap State
    // ========================================
    bool bScaleSnapEnabled = false;
    float ScaleSnapValue = DEFAULT_SCALE_SNAP_VALUE;

    // ========================================
    // Animation State
    // ========================================
    struct FViewportAnimation
    {
        bool bIsAnimating = false;
        float AnimationTime = 0.0f;
        float AnimationDuration = 0.2f;

        bool bSingleToQuad = true;

        SWindow* BackupRoot = nullptr;

        int32 PromotedViewportIndex = 0;

        float StartVRatio = 0.5f;
        float StartHRatio = 0.5f;
        float TargetVRatio = 0.5f;
        float TargetHRatio = 0.5f;
        int SavedMinChildSizeV = 4;
        int SavedMinChildSizeH = 4;
    };

    FViewportAnimation ViewportAnimation;

    // ========================================
    // App Reference
    // ========================================
    FAppWindow* AppWindow = nullptr;

    // ========================================
    // Helper Functions
    // ========================================
    void SyncRectsToViewports() const;
    void InitializeViewportAndClient();

    void UpdateViewportAnimation();
    static float EaseInOutCubic(float InT);
    void FinalizeSingleLayoutFromAnimation();
    void FinalizeFourSplitLayoutFromAnimation();
};
