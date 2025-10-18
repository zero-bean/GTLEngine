#pragma once
#include "LightComponent.h"
#include "CBufferTypes.h"
#include <array>

// Forward declarations
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;
class URenderer;
class UWorld;

// Cubemap face enumeration
enum class ECubeFace : uint8
{
    PositiveX = 0,  // +X
    NegativeX = 1,  // -X
    PositiveY = 2,  // +Y
    NegativeY = 3,  // -Y
    PositiveZ = 4,  // +Z
    NegativeZ = 5,  // -Z
    Count = 6
};

// Dynamic Ambient Light Probe using Spherical Harmonics
// Captures 6 directions and projects to L2 SH coefficients for ambient lighting
class UDynamicAmbientProbe : public ULightComponent
{
public:
    DECLARE_CLASS(UDynamicAmbientProbe, ULightComponent)

    UDynamicAmbientProbe();
    ~UDynamicAmbientProbe() override;

    // Component lifecycle
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime) override;
    virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

    // Serialization
    virtual void Serialize(bool bIsLoading, FComponentData& InOut) override;

    // SH Configuration
    void SetCaptureResolution(int32 InResolution);
    int32 GetCaptureResolution() const { return CaptureResolution; }

    void SetUpdateInterval(float InInterval) { UpdateInterval = InInterval; }
    float GetUpdateInterval() const { return UpdateInterval; }

    void SetSmoothingFactor(float InFactor) { SmoothingFactor = FMath::Clamp(InFactor, 0.0f, 1.0f); }
    float GetSmoothingFactor() const { return SmoothingFactor; }

    void SetSHIntensity(float InIntensity) { SHIntensity = InIntensity; }
    float GetSHIntensity() const { return SHIntensity; }

    // Get current SH coefficients
    const FSHAmbientLightBufferType& GetSHBuffer() const { return SHBuffer; }

    // Force immediate capture
    void ForceCapture();

private:
    // Initialize cubemap render targets
    void InitializeCubemapTargets();
    void ReleaseCubemapTargets();

    // Capture environment to cubemap faces
    void CaptureEnvironment(UWorld* World, URenderer* Renderer);

    // Project cubemap to SH coefficients
    void ProjectToSH();

    // Apply temporal smoothing
    void SmoothSH();

    // Get view matrix for each cubemap face
    FMatrix GetCubeFaceViewMatrix(ECubeFace Face) const;

    // Get projection matrix (90 degree FOV)
    FMatrix GetCubeFaceProjectionMatrix() const;

    // Convert cube UV to direction vector
    static FVector CubeUVToDir(ECubeFace Face, float U, float V);

    // Solid angle for texel
    static float TexelSolidAngle(int32 X, int32 Y, int32 Size);

    // SH basis functions (L2, 9 coefficients)
    static float SHBasis(int32 Index, const FVector& Dir);

private:
    // Capture configuration
    int32 CaptureResolution = 100;           // Cubemap face resolution (64x64)
    float UpdateInterval = 0.1f;            // Update every N seconds
    float SmoothingFactor = 0.1f;           // Temporal smoothing (0=no smooth, 1=instant)
    float SHIntensity = 1.0f;               // Global intensity multiplier

    // Update timing
    float TimeSinceLastCapture = 0.0f;

    // Cubemap render targets (6 faces)
    std::array<ID3D11Texture2D*, 6> CubemapTextures = {};
    std::array<ID3D11RenderTargetView*, 6> CubemapRTVs = {};
    std::array<ID3D11ShaderResourceView*, 6> CubemapSRVs = {};

    // Shared depth stencil for all faces
    ID3D11Texture2D* CubemapDepthTexture = nullptr;
    ID3D11DepthStencilView* CubemapDSV = nullptr;

    // SH coefficients
    FSHAmbientLightBufferType SHBuffer = {};      // Current frame SH
    FSHAmbientLightBufferType SHBufferPrev = {};  // Previous frame SH for smoothing

    // Radiance to irradiance conversion factors
    // A0 = π, A1 = 2π/3, A2 = π/4
    static constexpr float SH_A0 = 3.141592653589793f;
    static constexpr float SH_A1 = 2.0943951023931953f;  // 2π/3
    static constexpr float SH_A2 = 0.7853981633974483f;  // π/4

    // Whether resources are initialized
    bool bIsInitialized = false;
};
