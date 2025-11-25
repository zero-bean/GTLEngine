#pragma once

class FViewport;
class FViewportClient;
class UWorld;
class UParticleSystem;
class UParticleEmitter;

enum class EParticleDetailSelection : uint8
{
    None = 0,
    Required,
    Spawn,
    Module
};

class ParticleEditorState
{
public:
    FName Name;
    UWorld* World = nullptr;
    FViewport* Viewport = nullptr;
    FViewportClient* Client = nullptr;

    // Particle system and emitter references
    UParticleSystem* CurrentParticleSystem = nullptr;
    FString LoadedParticleSystemPath;
    int32 SelectedEmitterIndex = -1;

    // UI state
    bool bShowGrid = false;
    bool bShowAxis = true;
    bool bShowBounds = false;
    int32 ActiveLODLevel = 0;
    float ViewportBackgroundColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // RGBA

    // Playback state
    bool bIsPlaying = false;
    bool bIsLooping = true;
    float CurrentTime = 0.0f;
    float PlaybackSpeed = 1.0f;

    // Timeline state
    float TimelineScale = 10.0f;
    float TimelineOffset = 0.0f;
    bool bTimeChanged = false;

    // Emitter property editing state
    int32 SelectedPropertyIndex = -1;
    bool bShowCurveEditor = true;
    EParticleDetailSelection SelectedModuleSelection = EParticleDetailSelection::None;
    int32 SelectedModuleIndex = -1;

    // Curve editor state
    int32 SelectedCurveIndex = -1;
    int32 SelectedKeyframeIndex = -1;

    // Helper functions
    bool HasParticleSystem() const { return CurrentParticleSystem != nullptr; }
    
    UParticleEmitter* GetSelectedEmitter() const;
    int32 GetEmitterCount() const;
};
