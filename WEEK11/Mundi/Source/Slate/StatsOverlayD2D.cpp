#include "pch.h"

#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <dxgi1_2.h>

#include "StatsOverlayD2D.h"
#include "UIManager.h"
#include "MemoryManager.h"
#include "Picking.h"
#include "PlatformTime.h"
#include "DecalStatManager.h"
#include "TileCullingStats.h"
#include "LightStats.h"
#include "ShadowStats.h"
#include "SkinningStats.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

static inline void SafeRelease(IUnknown* p) { if (p) p->Release(); }

UStatsOverlayD2D& UStatsOverlayD2D::Get()
{
	static UStatsOverlayD2D Instance;
	return Instance;
}

void UStatsOverlayD2D::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InContext, IDXGISwapChain* InSwapChain)
{
	D3DDevice = InDevice;
	D3DContext = InContext;
	SwapChain = InSwapChain;
	bInitialized = (D3DDevice && D3DContext && SwapChain);
}

void UStatsOverlayD2D::Shutdown()
{
	// Release cached D2D/DWrite resources
	SafeRelease(TargetBmp);
	SafeRelease(BrushText);
	SafeRelease(BrushFill);
	SafeRelease(Dwrite);
	SafeRelease(D2dCtx);
	SafeRelease(D2dDevice);
	SafeRelease(D2dFactory);
	BackBufferW = BackBufferH = 0;

	D3DDevice = nullptr;
	D3DContext = nullptr;
	SwapChain = nullptr;
	bInitialized = false;
}

void UStatsOverlayD2D::EnsureInitialized()
{
    if (!bInitialized || !D3DDevice || !SwapChain)
        return;

    // Factory
    if (!D2dFactory)
    {
        D2D1_FACTORY_OPTIONS Opts{};
#ifdef _DEBUG
        Opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &Opts, (void**)&D2dFactory);
    }

    // Device + DeviceContext
    if (!D2dDevice || !D2dCtx)
    {
        IDXGIDevice* DxgiDevice = nullptr;
        if (SUCCEEDED(D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&DxgiDevice)))
        {
            if (!D2dDevice && D2dFactory)
            {
                D2dFactory->CreateDevice(DxgiDevice, &D2dDevice);
            }
            if (D2dDevice && !D2dCtx)
            {
                D2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &D2dCtx);
            }
        }
        SafeRelease(DxgiDevice);
    }

    // DWrite factory
    if (!Dwrite)
    {
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&Dwrite);
    }

    // Reusable brushes
    if (D2dCtx)
    {
        if (!BrushFill)
        {
            D2dCtx->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &BrushFill);
        }
        if (!BrushText)
        {
            D2dCtx->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &BrushText);
        }
    }
}

void UStatsOverlayD2D::ReleaseD2DTarget()
{
    SafeRelease(TargetBmp);
    BackBufferW = BackBufferH = 0;
}

void UStatsOverlayD2D::RecreateTargetBitmapIfNeeded()
{
    if (!SwapChain || !D2dCtx)
        return;

    IDXGISurface* Surface = nullptr;
    if (FAILED(SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&Surface)))
        return;

    DXGI_SURFACE_DESC SD{};
    Surface->GetDesc(&SD);

    bool bNeedNew = (TargetBmp == nullptr) || (SD.Width != BackBufferW) || (SD.Height != BackBufferH);
    if (bNeedNew)
    {
        ReleaseD2DTarget();

        D2D1_BITMAP_PROPERTIES1 BmpProps = {};
        BmpProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        BmpProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        BmpProps.dpiX = 96.0f;
        BmpProps.dpiY = 96.0f;
        BmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        if (SUCCEEDED(D2dCtx->CreateBitmapFromDxgiSurface(Surface, &BmpProps, &TargetBmp)))
        {
            BackBufferW = SD.Width;
            BackBufferH = SD.Height;
        }
    }

    SafeRelease(Surface);
}

static void DrawTextBlock(
	ID2D1DeviceContext* InD2dCtx,
	IDWriteFactory* InDwrite,
	ID2D1SolidColorBrush* InBrushFill,
	ID2D1SolidColorBrush* InBrushText,
	const wchar_t* InText,
	const D2D1_RECT_F& InRect,
	float InFontSize,
	D2D1::ColorF InBgColor,
	D2D1::ColorF InTextColor)
{
	if (!InD2dCtx || !InDwrite || !InText) return;

	// Reuse brushes, just change colors each draw
	if (InBrushFill) InBrushFill->SetColor(InBgColor);
	if (InBrushText) InBrushText->SetColor(InTextColor);

	InD2dCtx->FillRectangle(InRect, InBrushFill);

	IDWriteTextFormat* Format = nullptr;
	InDwrite->CreateTextFormat(
		L"Segoe UI",
		nullptr,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		InFontSize,
		L"en-us",
		&Format);

	if (Format)
	{
		Format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		Format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		InD2dCtx->DrawTextW(
			InText,
			static_cast<UINT32>(wcslen(InText)),
			Format,
			InRect,
			InBrushText);
		Format->Release();
	}
}

void UStatsOverlayD2D::Draw()
{
    if (!bInitialized || (!bShowFPS && !bShowMemory && !bShowPicking && !bShowDecal && !bShowTileCulling && !bShowLights && !bShowShadow && !bShowSkinning) || !SwapChain)
        return;

    // Create once, reuse every frame
    EnsureInitialized();
    RecreateTargetBitmapIfNeeded();
    if (!D2dCtx || !TargetBmp || !Dwrite)
        return;

    D2dCtx->SetTarget(TargetBmp);

    D2dCtx->BeginDraw();
	const float Margin = 12.0f;
	const float Space = 8.0f;   // 패널간의 간격
	const float PanelWidth = 200.0f;
	const float PanelHeight = 48.0f;
	float NextY = 70.0f;

	if (bShowFPS)
	{
		float Dt = UUIManager::GetInstance().GetDeltaTime();
		float Fps = Dt > 0.0f ? (1.0f / Dt) : 0.0f;
		float Ms = Dt * 1000.0f;

		wchar_t Buf[128];
		swprintf_s(Buf, L"FPS: %.1f\nFrame time: %.2f ms", Fps, Ms);

        D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PanelHeight);
        DrawTextBlock(
            D2dCtx, Dwrite, BrushFill, BrushText, Buf, rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::Yellow));

		NextY += PanelHeight + Space;
	}

	if (bShowPicking)
	{
		// Build the entire block in one Buffer to avoid overwriting previous lines
		wchar_t Buf[256];
		double LastMs = FWindowsPlatformTime::ToMilliseconds(CPickingSystem::GetLastPickTime());
		double TotalMs = FWindowsPlatformTime::ToMilliseconds(CPickingSystem::GetTotalPickTime());
		uint32 Count = CPickingSystem::GetPickCount();
		double AvgMs = (Count > 0) ? (TotalMs / (double)Count) : 0.0;
		swprintf_s(Buf, L"Pick Count: %u\nLast: %.3f ms\nAvg: %.3f ms\nTotal: %.3f ms", Count, LastMs, AvgMs, TotalMs);

		// Increase panel height to fit multiple lines
		const float PickPanelHeight = 96.0f;
        D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PickPanelHeight);
        DrawTextBlock(
            D2dCtx, Dwrite, BrushFill, BrushText, Buf, rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::SkyBlue));

		NextY += PickPanelHeight + Space;
	}

	if (bShowMemory)
	{
		double Mb = static_cast<double>(FMemoryManager::TotalAllocationBytes) / (1024.0 * 1024.0);

		wchar_t Buf[128];
		swprintf_s(Buf, L"Memory: %.1f MB\nAllocs: %u", Mb, FMemoryManager::TotalAllocationCount);

        D2D1_RECT_F Rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PanelHeight);
        DrawTextBlock(
            D2dCtx, Dwrite, BrushFill, BrushText, Buf, Rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::LightGreen));

		NextY += PanelHeight + Space;
	}

	if (bShowDecal)
	{
		// 1. FDecalStatManager로부터 통계 데이터를 가져옵니다.
		uint32_t TotalCount = FDecalStatManager::GetInstance().GetTotalDecalCount();
		//uint32_t VisibleDecalCount = FDecalStatManager::GetInstance().GetVisibleDecalCount();
		uint32_t AffectedMeshCount = FDecalStatManager::GetInstance().GetAffectedMeshCount();
		double TotalTime = FDecalStatManager::GetInstance().GetDecalPassTimeMS();
		double AverageTimePerDecal = FDecalStatManager::GetInstance().GetAverageTimePerDecalMS();
		double AverageTimePerDraw = FDecalStatManager::GetInstance().GetAverageTimePerDrawMS();

		// 2. 출력할 문자열 버퍼를 만듭니다.
		wchar_t Buf[256];
		swprintf_s(Buf, L"[Decal Stats]\nTotal: %u\nAffectedMesh: %u\n전체 소요 시간: %.3f ms\nAvg/Decal: %.3f ms\nAvg/Mesh: %.3f ms",
			TotalCount,
			AffectedMeshCount,
			TotalTime,
			AverageTimePerDecal,
			AverageTimePerDraw);

		// 3. 텍스트를 여러 줄 표시해야 하므로 패널 높이를 늘립니다.
		const float decalPanelHeight = 140.0f;
        D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + decalPanelHeight);

        // 4. DrawTextBlock 함수를 호출하여 화면에 그립니다. 색상은 구분을 위해 주황색(Orange)으로 설정합니다.
        DrawTextBlock(
            D2dCtx, Dwrite, BrushFill, BrushText, Buf, rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::Orange));

		NextY += decalPanelHeight + Space;
	}

	if (bShowTileCulling)
	{
		// 1. FTileCullingStatManager로부터 통계 데이터를 가져옵니다.
		const FTileCullingStats& TileStats = FTileCullingStatManager::GetInstance().GetStats();

		// 2. 출력할 문자열 버퍼를 만듭니다.
		wchar_t Buf[512];
		swprintf_s(Buf, L"[Tile Culling Stats]\nTiles: %u x %u (%u)\nLights: %u (P:%u S:%u)\nMin/Avg/Max: %u / %.1f / %u\nCulling Eff: %.1f%%\nBuffer: %u KB",
			TileStats.TileCountX,
			TileStats.TileCountY,
			TileStats.TotalTileCount,
			TileStats.TotalLights,
			TileStats.TotalPointLights,
			TileStats.TotalSpotLights,
			TileStats.MinLightsPerTile,
			TileStats.AvgLightsPerTile,
			TileStats.MaxLightsPerTile,
			TileStats.CullingEfficiency,
			TileStats.LightIndexBufferSizeBytes / 1024);

		// 3. 텍스트를 여러 줄 표시해야 하므로 패널 높이를 늘립니다.
		const float tilePanelHeight = 160.0f;
        D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + tilePanelHeight);

        // 4. DrawTextBlock 함수를 호출하여 화면에 그립니다. 색상은 구분을 위해 cyan으로 설정합니다.
        DrawTextBlock(
            D2dCtx, Dwrite, BrushFill, BrushText, Buf, rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::Cyan));

		NextY += tilePanelHeight + Space;
	}

	if (bShowLights)
	{
		// 1. FLightStatManager로부터 통계 데이터를 가져옵니다.
		const FLightStats& LightStats = FLightStatManager::GetInstance().GetStats();

		// 2. 출력할 문자열 버퍼를 만듭니다.
		wchar_t Buf[512];
		swprintf_s(Buf, L"[Light Stats]\nTotal Lights: %u\n  Point: %u\n  Spot: %u\n  Directional: %u\n  Ambient: %u",
			LightStats.TotalLights,
			LightStats.TotalPointLights,
			LightStats.TotalSpotLights,
			LightStats.TotalDirectionalLights,
			LightStats.TotalAmbientLights);

		// 3. 텍스트를 여러 줄 표시해야 하므로 패널 높이를 늘립니다.
		const float lightPanelHeight = 140.0f;
        D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + lightPanelHeight);

        // 4. DrawTextBlock 함수를 호출하여 화면에 그립니다. 색상은 구분을 위해 보라색(Purple)으로 설정합니다.
        DrawTextBlock(
            D2dCtx, Dwrite, BrushFill, BrushText, Buf, rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::Violet));

		NextY += lightPanelHeight + Space;
	}

	if (bShowShadow)
	{
		// 1. FShadowStatManager로부터 통계 데이터를 가져옵니다.
		const FShadowStats& ShadowStats = FShadowStatManager::GetInstance().GetStats();

		// 2. 출력할 문자열 버퍼를 만듭니다.
		wchar_t Buf[512];
		swprintf_s(Buf, L"[Shadow Stats]\nShadow Lights: %u\n  Point: %u\n  Spot: %u\n  Directional: %u\n\nAtlas 2D: %u x %u (%.1f MB)\nAtlas Cube: %u x %u x %u (%.1f MB)\n\nTotal Memory: %.1f MB",
			ShadowStats.TotalShadowCastingLights,
			ShadowStats.ShadowCastingPointLights,
			ShadowStats.ShadowCastingSpotLights,
			ShadowStats.ShadowCastingDirectionalLights,
			ShadowStats.ShadowAtlas2DSize,
			ShadowStats.ShadowAtlas2DSize,
			ShadowStats.ShadowAtlas2DMemoryMB,
			ShadowStats.ShadowAtlasCubeSize,
			ShadowStats.ShadowAtlasCubeSize,
			ShadowStats.ShadowCubeArrayCount,
			ShadowStats.ShadowAtlasCubeMemoryMB,
			ShadowStats.TotalShadowMemoryMB);

		// 3. 텍스트를 여러 줄 표시해야 하므로 패널 높이를 늘립니다.
		const float shadowPanelHeight = 260.0f;
        D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + shadowPanelHeight);

        // 4. DrawTextBlock 함수를 호출하여 화면에 그립니다. 색상은 구분을 위해 한색(Magenta)으로 설정합니다.
        DrawTextBlock(
            D2dCtx, Dwrite, BrushFill, BrushText, Buf, rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::DeepPink));

		NextY += shadowPanelHeight + Space;


        rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + 40);

        // 4. DrawTextBlock 함수를 호출하여 화면에 그립니다. 색상은 구분을 위해 한색(Magenta)으로 설정합니다.
        DrawTextBlock(
            D2dCtx, Dwrite, BrushFill, BrushText, FScopeCycleCounter::GetTimeProfile("ShadowMapPass").GetConstWChar_tWithKey("ShadowMapPass"), rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::DeepPink));

		NextY += shadowPanelHeight + Space;
	}

	if (bShowSkinning)
	{
		// 1. FSkinningStats로부터 통계 데이터를 가져옵니다.
		const FSkinningStats& SkinningStats = FSkinningStats::GetInstance();

		// 2. CPU/GPU 평균 시간 계산
		double cpuAvgTime = SkinningStats.GetAverageCPUSkinningTime();
		double gpuAvgTime = SkinningStats.GetAverageGPUSkinningTime();
		double cpuLastTime = SkinningStats.GetLastCPUSkinningTime();
		double gpuLastTime = SkinningStats.GetLastGPUSkinningTime();

		// 3. 출력할 문자열 버퍼를 만듭니다.
		wchar_t Buf[512];
		swprintf_s(Buf, L"[Skinning Performance]\n\n"
			L"CPU Skinning:\n"
			L"  Current: %.3f ms\n"
			L"  Average: %.3f ms (60f)\n\n"
			L"GPU Skinning:\n"
			L"  Current: %.3f ms\n"
			L"  Average: %.3f ms (60f)\n\n"
			L"Speedup: %.2fx",
			cpuLastTime,
			cpuAvgTime,
			gpuLastTime,
			gpuAvgTime,
			(gpuAvgTime > 0.001) ? (cpuAvgTime / gpuAvgTime) : 0.0);

		// 4. 텍스트를 여러 줄 표시해야 하므로 패널 높이를 늘립니다.
		const float skinningPanelHeight = 220.0f;
        D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + skinningPanelHeight);

        // 5. DrawTextBlock 함수를 호출하여 화면에 그립니다. 색상은 구분을 위해 주황색(Orange)으로 설정합니다.
        DrawTextBlock(
            D2dCtx, Dwrite, BrushFill, BrushText, Buf, rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::Orange));

		NextY += skinningPanelHeight + Space;
	}

    D2dCtx->EndDraw();
    D2dCtx->SetTarget(nullptr);

    FScopeCycleCounter::TimeProfileInit();
}

void UStatsOverlayD2D::SetShowFPS(bool b)
{
	bShowFPS = b;
}

void UStatsOverlayD2D::SetShowMemory(bool b)
{
	bShowMemory = b;
}

void UStatsOverlayD2D::SetShowPicking(bool b)
{
	bShowPicking = b;
}

void UStatsOverlayD2D::SetShowDecal(bool b)
{
	bShowDecal = b;
}

void UStatsOverlayD2D::ToggleFPS()
{
	bShowFPS = !bShowFPS;
}

void UStatsOverlayD2D::ToggleMemory()
{
	bShowMemory = !bShowMemory;
}

void UStatsOverlayD2D::TogglePicking()
{
	bShowPicking = !bShowPicking;
}

void UStatsOverlayD2D::ToggleDecal()
{
	bShowDecal = !bShowDecal;
}

void UStatsOverlayD2D::SetShowTileCulling(bool b)
{
	bShowTileCulling = b;
}

void UStatsOverlayD2D::ToggleTileCulling()
{
	bShowTileCulling = !bShowTileCulling;
}

void UStatsOverlayD2D::SetShowLights(bool b)
{
	bShowLights = b;
}

void UStatsOverlayD2D::ToggleLights()
{
	bShowLights = !bShowLights;
}

void UStatsOverlayD2D::SetShowShadow(bool b)
{
	bShowShadow = b;
}

void UStatsOverlayD2D::ToggleShadow()
{
	bShowShadow = !bShowShadow;
}

void UStatsOverlayD2D::SetShowSkinning(bool b)
{
	bShowSkinning = b;
}

void UStatsOverlayD2D::ToggleSkinning()
{
	bShowSkinning = !bShowSkinning;
}
