#include "pch.h"
#include "Core/Public/PlatformTime.h"
#include "Global/Memory.h"
#include "Global/Types.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"

#include "Core/Public/ScopeCycleCounter.h"
IMPLEMENT_SINGLETON_CLASS_BASE(UStatOverlay)

UStatOverlay::UStatOverlay() {}
UStatOverlay::~UStatOverlay() = default;

void UStatOverlay::Initialize()
{
	auto* DeviceResources = URenderer::GetInstance().GetDeviceResources();
	D2DFactory = DeviceResources->GetD2DFactory();
	DWriteFactory = DeviceResources->GetDWriteFactory();

	if (DWriteFactory)
	{
		DWriteFactory->CreateTextFormat(
			L"monospace",				// Font-name
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			15.0f,						// Font-size
			L"en-us",
			&TextFormat
		);
	}
	CreateRenderTarget();
}

void UStatOverlay::PreResize()
{
	if (TextBrush)		 { TextBrush->Release(); TextBrush = nullptr; }
	if (D2DRenderTarget) { D2DRenderTarget->Release(); D2DRenderTarget = nullptr; }
}

void UStatOverlay::OnResize()
{
	CreateRenderTarget();	// Recreate only back buffer dependent resources
}

void UStatOverlay::Release()
{
	if (TextBrush)		 { TextBrush->Release(); TextBrush = nullptr; }
	if (D2DRenderTarget) { D2DRenderTarget->Release(); D2DRenderTarget = nullptr; }
	if (TextFormat)		 { TextFormat->Release(); TextFormat = nullptr; }

	// Factories are managed by the Renderer
	D2DFactory = nullptr;
	DWriteFactory = nullptr;
}

void UStatOverlay::Render()
{
	TIME_PROFILE(Run)
	if (!D2DRenderTarget) return;

	D2DRenderTarget->BeginDraw();

	if (IsStatEnabled(EStatType::FPS))		{ RenderFPS(); }
	if (IsStatEnabled(EStatType::Memory))	{ RenderMemory(); }

	D2DRenderTarget->EndDraw();
	TIME_PROFILE_END(Run)
}

void UStatOverlay::CreateRenderTarget()
{
	if (D2DRenderTarget) { D2DRenderTarget->Release(); D2DRenderTarget = nullptr; }
	if (TextBrush) { TextBrush->Release(); TextBrush = nullptr; }

	auto* DeviceResources = URenderer::GetInstance().GetDeviceResources();
	if (!DeviceResources || !D2DFactory) return;

	ID3D11Texture2D* BackBuffer = nullptr;
	HRESULT hr = DeviceResources->GetSwapChain()->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
	if (FAILED(hr) || !BackBuffer)
		return;

	IDXGISurface* DxgiSurface = nullptr;
	hr = BackBuffer->QueryInterface(IID_PPV_ARGS(&DxgiSurface));
	if (FAILED(hr) || !DxgiSurface)
		return;

	D2D1_RENDER_TARGET_PROPERTIES Props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
	);

	if (SUCCEEDED(D2DFactory->CreateDxgiSurfaceRenderTarget(DxgiSurface, &Props, &D2DRenderTarget)))
	{
		if (D2DRenderTarget && !TextBrush)
		{
			D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &TextBrush);
		}
	}

	DxgiSurface->Release();
	BackBuffer->Release();
}

void UStatOverlay::RenderFPS()
{
	auto& TimeManager = UTimeManager::GetInstance();
	CurrentFPS = TimeManager.GetFPS();
	FrameTime = (CurrentFPS > 0.0f) ? (1000.0f / CurrentFPS) : 0.0f;

	char FpsBuffer[64];
	sprintf_s(FpsBuffer, sizeof(FpsBuffer), "FPS: %.1f (%.2f ms)", CurrentFPS, FrameTime);
	FString FpsText = FpsBuffer;

	float R = 0.5f, G = 1.0f, B = 0.5f;
	if (CurrentFPS < 30.0f) { R = 1.0f; G = 0.0f; B = 0.0f; }
	else if (CurrentFPS < 60.0f) { R = 1.0f; G = 1.0f; B = 0.0f; }

	RenderText(FpsText, OverlayX, OverlayY, R, G, B);

	// --- Picking Stats Text ---
	char PickBuffer[128];
	sprintf_s(PickBuffer, sizeof(PickBuffer),
		"Picking Time %f ms : Num Attempts %u : Cumulative Time %f ms : Average Time %f ms",
		LastPickingTime,
		NumPickingAttempts,
		CumulativePickingTime,
		CumulativePickingTime / static_cast<float>(NumPickingAttempts));

	FString PickText = PickBuffer;

	// Render below the FPS text (shift Y down by ~20 pixels)
	RenderText(PickText, OverlayX, OverlayY + 20.0f, 1.0f, 1.0f, 1.0f);
}

void UStatOverlay::RenderMemory()
{
	float MemoryMB = static_cast<float>(TotalAllocationBytes) / (1024.0f * 1024.0f);

	char MemoryBuffer[64];
	sprintf_s(MemoryBuffer, sizeof(MemoryBuffer), "Memory: %.1f MB (%u objects)", MemoryMB, TotalAllocationCount);
	FString MemoryText = MemoryBuffer;

	float OffsetY = IsStatEnabled(EStatType::FPS) ? 20.0f : 0.0f;
	RenderText(MemoryText, OverlayX, OverlayY + OffsetY, 1.0f, 1.0f, 0.0f);
}

void UStatOverlay::RenderText(const FString& Text, float X, float Y, float R, float G, float B)
{
	if (!D2DRenderTarget || !TextBrush || !TextFormat) return;

	TextBrush->SetColor(D2D1::ColorF(R, G, B));
	std::wstring wText = ToWString(Text);

	D2D1_RECT_F layoutRect = D2D1::RectF(X, Y, X + 400.0f, Y + 20.0f);
	D2DRenderTarget->DrawText(
		wText.c_str(),
		static_cast<UINT32>(wText.length()),
		TextFormat,
		&layoutRect,
		TextBrush
	);
}

std::wstring UStatOverlay::ToWString(const FString& InStr)
{
	if (InStr.empty()) return std::wstring();

	int SizeNeeded = MultiByteToWideChar(CP_UTF8, 0, InStr.c_str(), (int)InStr.size(), NULL, 0);
	std::wstring wStr(SizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, InStr.c_str(), (int)InStr.size(), &wStr[0], SizeNeeded);
	return wStr;
}

void UStatOverlay::EnableStat(EStatType Type)
{
	StatMask |= static_cast<uint8>(Type);
}

void UStatOverlay::DisableStat(EStatType Type)
{
	StatMask &= ~static_cast<uint8>(Type);
}

void UStatOverlay::SetStatType(EStatType Type)
{
	StatMask = static_cast<uint8>(Type);
}

bool UStatOverlay::IsStatEnabled(EStatType Type) const
{
	return (StatMask & static_cast<uint8>(Type)) != 0;
}
