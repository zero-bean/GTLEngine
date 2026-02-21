#include "stdafx.h"
#include "UTimeManager.h"
#include "UClass.h"
IMPLEMENT_UCLASS(UTimeManager, UEngineSubsystem)
UTimeManager::UTimeManager()
    : deltaTime(0.0)
    , totalTime(0.0)
    , targetFPS(60)
    , targetFrameTime(1000.0 / 60.0)
{
    // Initialize LARGE_INTEGER structures
    ZeroMemory(&frequency, sizeof(frequency));
    ZeroMemory(&startTime, sizeof(startTime));
    ZeroMemory(&lastFrameTime, sizeof(lastFrameTime));
}

UTimeManager::~UTimeManager()
{
    // Nothing specific to clean up
}

bool UTimeManager::Initialize(int32 fps)
{
    // Get the high-resolution performance counter frequency
    if (!QueryPerformanceFrequency(&frequency))
    {
        // Fallback: use GetTickCount() based timing
        OutputDebugStringA("Warning: High-resolution performance counter not available\n");
        return false;
    }

    // Set target FPS
    SetTargetFPS(fps);

    // Initialize timing
    QueryPerformanceCounter(&startTime);
    lastFrameTime = startTime;

    // Reset time tracking
    deltaTime = 0.0;
    totalTime = 0.0;

    return true;
}

void UTimeManager::BeginFrame()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    // Calculate delta time in seconds
    deltaTime = static_cast<double>(currentTime.QuadPart - lastFrameTime.QuadPart) /
        static_cast<double>(frequency.QuadPart);

    // Clamp delta time to prevent huge jumps (e.g., when debugging)
    // Maximum frame time: 1/10 second (10 FPS minimum)
    if (deltaTime > 0.1)
    {
        deltaTime = 0.1;
    }

    // Update total time
    totalTime += deltaTime;

    lastFrameTime = currentTime;
}

void UTimeManager::EndFrame()
{
}

void UTimeManager::WaitForTargetFrameTime()
{
    if (targetFrameTime <= 0.0)
        return;

    LARGE_INTEGER currentTime;
    double elapsedTime;

    do
    {
        // Yield CPU time to other processes
        Sleep(0);

        // Check elapsed time
        QueryPerformanceCounter(&currentTime);
        elapsedTime = static_cast<double>(currentTime.QuadPart - lastFrameTime.QuadPart) * 1000.0 /
            static_cast<double>(frequency.QuadPart);

    } while (elapsedTime < targetFrameTime);
}

void UTimeManager::SetTargetFPS(int32 fps)
{
    if (fps <= 0)
    {
        targetFPS = 60; // Default fallback
        targetFrameTime = 0.0; // No frame rate limiting
    }
    else
    {
        targetFPS = fps;
        targetFrameTime = 1000.0 / static_cast<double>(fps); // Convert to milliseconds
    }
}