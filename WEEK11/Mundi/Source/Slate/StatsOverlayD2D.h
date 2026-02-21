#pragma once

#include <d3d11.h>
#include <dxgi.h>

// Forward declarations to keep this header light
struct ID2D1Factory1;
struct ID2D1Device;
struct ID2D1DeviceContext;
struct IDWriteFactory;
struct ID2D1Bitmap1;
struct ID2D1SolidColorBrush;

class UStatsOverlayD2D
{
public:
    static UStatsOverlayD2D& Get();

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain);
	void Shutdown();
    void Draw();

    void SetShowFPS(bool b);
    void SetShowMemory(bool b);
    void SetShowPicking(bool b);
    void SetShowDecal(bool b);
    void SetShowTileCulling(bool b);
    void SetShowLights(bool b);
    void SetShowShadow(bool b);
    void SetShowSkinning(bool b);
    void ToggleFPS();
    void ToggleMemory();
    void TogglePicking();
    void ToggleDecal();
    void ToggleTileCulling();
    void ToggleLights();
    void ToggleShadow();
    void ToggleSkinning();
    bool IsFPSVisible() const { return bShowFPS; }
    bool IsMemoryVisible() const { return bShowMemory; }
    bool IsPickingVisible() const { return bShowPicking; }
    bool IsDecalVisible() const { return bShowDecal; }
    bool IsTileCullingVisible() const { return bShowTileCulling; }
    bool IsLightsVisible() const { return bShowLights; }
    bool IsShadowVisible() const { return bShowShadow; }
    bool IsSkinningVisible() const { return bShowSkinning; }

private:
    UStatsOverlayD2D() = default;
    ~UStatsOverlayD2D() = default;
    UStatsOverlayD2D(const UStatsOverlayD2D&) = delete;
    UStatsOverlayD2D& operator=(const UStatsOverlayD2D&) = delete;

    void EnsureInitialized();
    void ReleaseD2DTarget();
    void RecreateTargetBitmapIfNeeded();

private:
    bool bInitialized = false;
    bool bShowFPS = false;
    bool bShowMemory = false;
    bool bShowPicking = false;
    bool bShowDecal = false;
    bool bShowTileCulling = false;
    bool bShowShadow = false;
    bool bShowLights = false;
    bool bShowSkinning = false;

    ID3D11Device* D3DDevice = nullptr;
    ID3D11DeviceContext* D3DContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;

    // Cached D2D/DWrite resources (created once, reused every frame)
    ID2D1Factory1* D2dFactory = nullptr;
    ID2D1Device* D2dDevice = nullptr;
    ID2D1DeviceContext* D2dCtx = nullptr;
    IDWriteFactory* Dwrite = nullptr;
    ID2D1Bitmap1* TargetBmp = nullptr; // Backbuffer-wrapped target bitmap
    ID2D1SolidColorBrush* BrushFill = nullptr; // reusable brush (color set per use)
    ID2D1SolidColorBrush* BrushText = nullptr; // reusable brush (color set per use)

    // Track backbuffer size to recreate target on resize
    UINT BackBufferW = 0;
    UINT BackBufferH = 0;
};
