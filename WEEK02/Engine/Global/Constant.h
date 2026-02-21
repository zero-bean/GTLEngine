#pragma once

namespace Render
{
	// Screen Size
	constexpr int32 INIT_SCREEN_WIDTH = 1600;
	constexpr int32 INIT_SCREEN_HEIGHT = 900;
}

namespace Time
{
	// Time Sample Count
	constexpr int32 FPS_SAMPLE_COUNT = 60;
}

constexpr float Pi = 3.141592f;
constexpr uint64 KILO = 1024;
constexpr uint64 MEGA = 1024 * 1024;

constexpr float CameraSpeed = 6.0f;
constexpr float KeySensitivityDegPerPixel = 0.05f;
