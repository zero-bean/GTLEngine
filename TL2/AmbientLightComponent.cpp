#include "pch.h"
#include "AmbientLightComponent.h"
#include "Renderer.h"
#include "World.h"
#include "D3D11RHI.h"
#include "SMultiViewportWindow.h"
#include "SceneLoader.h"
#include <d3d11.h>

IMPLEMENT_CLASS(UAmbientLightComponent)

UAmbientLightComponent::UAmbientLightComponent()
{
    bCanEverTick = true;
    bIsRender = true;  // This component doesn't render itself, it affects ambient lighting

    // Initialize SH buffers to zero
    for (int32 i = 0; i < 9; ++i)
    {
        SHBuffer.SHCoefficients[i] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
        SHBufferPrev.SHCoefficients[i] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    SHBuffer.Intensity = 1.0f;
    SHBufferPrev.Intensity = 1.0f;
    InitializeCubemapTargets();
}

UAmbientLightComponent::~UAmbientLightComponent()
{
    ReleaseCubemapTargets();
}

void UAmbientLightComponent::BeginPlay()
{
    Super_t::BeginPlay();

    // Capture once at initialization for static ambient lighting
    ForceCapture();
}

void UAmbientLightComponent::TickComponent(float DeltaTime)
{
    Super_t::TickComponent(DeltaTime);

    // Static mode: no updates after initial capture
    // If you need dynamic updates, uncomment the code below:

    
    //TimeSinceLastCapture += DeltaTime;

    //// Update at specified interval
    //if (TimeSinceLastCapture >= UpdateInterval)
    //{
    //    TimeSinceLastCapture = 0.0f;
    //    ForceCapture();
    //}
    
}

void UAmbientLightComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
    ReleaseCubemapTargets();
    Super_t::EndPlay(EEndPlayReason::Destroyed);
}

void UAmbientLightComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    Super_t::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        // Load probe settings from serialized data
        AmbientData = InOut.AmbientLightProperty;
    }
    else
    {
        // Save probe settings
        InOut.AmbientLightProperty = AmbientData;
    }
}

void UAmbientLightComponent::SetCaptureResolution(int32 InResolution)
{
    if (InResolution != CaptureResolution)
    {
        CaptureResolution = InResolution;

        // Recreate render targets with new resolution
        if (bIsInitialized)
        {
            ReleaseCubemapTargets();
            InitializeCubemapTargets();
        }
    }
}

void UAmbientLightComponent::ForceCapture()
{
    if (!bIsInitialized || !Owner)
        return;

    UWorld* World = Owner->GetWorld();
    if (!World)
        return;

    URenderer* Renderer = World->GetRenderer();
    if (!Renderer)
        return;

    // Capture environment
    CaptureEnvironment(World, Renderer); // 6면체 캡처를 진행합니다.

    // Project to SH
    ProjectToSH(); // 큐브맵 → SH 계수 변환

    // Note: Smoothing is skipped for static capture to avoid dimming
    // (Smoothing is only useful for dynamic updates to prevent flickering)
   // SmoothSH();
}

void UAmbientLightComponent::InitializeCubemapTargets()
{
    if (bIsInitialized)
        return;

    //if (!Owner || !Owner->GetWorld())
    //    return;

    URenderer* Renderer = GEngine->GetWorld()->GetRenderer();
    if (!Renderer)
        return;

    ID3D11Device* Device = Renderer->GetRHIDevice()->GetDevice();
    if (!Device)
        return;

    // Create HDR render targets for 6 cubemap faces
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = CaptureResolution;
    texDesc.Height = CaptureResolution;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;  // HDR format
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    for (int32 i = 0; i < 6; ++i)
    {
        HRESULT hr = Device->CreateTexture2D(&texDesc, nullptr, &CubemapTextures[i]);
        if (FAILED(hr))
        {
            ReleaseCubemapTargets();
            return;
        }

        hr = Device->CreateRenderTargetView(CubemapTextures[i], nullptr, &CubemapRTVs[i]);
        if (FAILED(hr))
        {
            ReleaseCubemapTargets();
            return;
        }

        hr = Device->CreateShaderResourceView(CubemapTextures[i], nullptr, &CubemapSRVs[i]);
        if (FAILED(hr))
        {
            ReleaseCubemapTargets();
            return;
        }
    }

    // Create shared depth stencil
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = CaptureResolution;
    depthDesc.Height = CaptureResolution;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;

    HRESULT hr = Device->CreateTexture2D(&depthDesc, nullptr, &CubemapDepthTexture);
    if (FAILED(hr))
    {
        ReleaseCubemapTargets();
        return;
    }

    hr = Device->CreateDepthStencilView(CubemapDepthTexture, nullptr, &CubemapDSV);
    if (FAILED(hr))
    {
        ReleaseCubemapTargets();
        return;
    }

    bIsInitialized = true;
}

void UAmbientLightComponent::ReleaseCubemapTargets()
{
    for (int32 i = 0; i < 6; ++i)
    {
        if (CubemapSRVs[i]) { CubemapSRVs[i]->Release(); CubemapSRVs[i] = nullptr; }
        if (CubemapRTVs[i]) { CubemapRTVs[i]->Release(); CubemapRTVs[i] = nullptr; }
        if (CubemapTextures[i]) { CubemapTextures[i]->Release(); CubemapTextures[i] = nullptr; }
    }

    if (CubemapDSV) { CubemapDSV->Release(); CubemapDSV = nullptr; }
    if (CubemapDepthTexture) { CubemapDepthTexture->Release(); CubemapDepthTexture = nullptr; }

    bIsInitialized = false;
}

void UAmbientLightComponent::CaptureEnvironment(UWorld* World, URenderer* Renderer)
{
    if (!bIsInitialized)
        return;

    ID3D11DeviceContext* Context = Renderer->GetRHIDevice()->GetDeviceContext();
    if (!Context)
        return;

    // Save current render targets
    ID3D11RenderTargetView* OldRTV = nullptr;
    ID3D11DepthStencilView* OldDSV = nullptr;
    Context->OMGetRenderTargets(1, &OldRTV, &OldDSV);

    // Save current viewport
    UINT NumViewports = 1;
    D3D11_VIEWPORT OldViewport;
    Context->RSGetViewports(&NumViewports, &OldViewport);

    // Save current view mode and switch to Unlit for capturing base colors
    EViewModeIndex OldViewMode = Renderer->GetCurrentViewMode();
    Renderer->SetShadingModel(ELightShadingModel::Unlit);

    // Setup cubemap viewport
    D3D11_VIEWPORT CubeViewport = {};
    CubeViewport.Width = static_cast<float>(CaptureResolution);
    CubeViewport.Height = static_cast<float>(CaptureResolution);
    CubeViewport.MinDepth = 0.0f;
    CubeViewport.MaxDepth = 1.0f;
    CubeViewport.TopLeftX = 0.0f;
    CubeViewport.TopLeftY = 0.0f;

    FVector ProbeLocation = GetWorldLocation();

    // Get active viewport (for show flags and rendering)
    FViewport* ActiveViewport = nullptr;
    if (World->GetMultiViewportWindow())
    {
        ActiveViewport = World->GetMainViewport()->GetViewport();
    }

    // Render each cubemap face
    for (int32 i = 0; i < 6; ++i)
    {
        ECubeFace Face = static_cast<ECubeFace>(i);

        // Set render targets
        Context->OMSetRenderTargets(1, &CubemapRTVs[i], CubemapDSV);
        Context->RSSetViewports(1, &CubeViewport);

        // Clear
        float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        Context->ClearRenderTargetView(CubemapRTVs[i], ClearColor);
        Context->ClearDepthStencilView(CubemapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Get matrices for this face
        FMatrix ViewMatrix = GetCubeFaceViewMatrix(Face);
        FMatrix ProjMatrix = GetCubeFaceProjectionMatrix();

        // Render scene from probe position to this cubemap face
        if (ActiveViewport)
        {
            Renderer->RenderSceneToCubemapFace(World, ViewMatrix, ProjMatrix, ProbeLocation, ActiveViewport);
        }
    }

    // Restore view mode
    Renderer->SetShadingModel(ELightShadingModel::BlinnPhong);

    // Restore render targets and viewport
    Context->OMSetRenderTargets(1, &OldRTV, OldDSV);
    Context->RSSetViewports(1, &OldViewport);

    if (OldRTV) OldRTV->Release();
    if (OldDSV) OldDSV->Release();
}

void UAmbientLightComponent::ProjectToSH()
{
    if (!bIsInitialized)
        return;

    UWorld* World = Owner->GetWorld();
    if (!World)
        return;

    URenderer* Renderer = World->GetRenderer();
    if (!Renderer)
        return;

    ID3D11DeviceContext* Context = Renderer->GetRHIDevice()->GetDeviceContext();
    if (!Context)
        return;

    // ✅ 임시 테스트: 텍스처 읽기를 건너뛰고 임의값으로 SH 계수 설정
    // 실내 환경을 시뮬레이션하는 전형적인 SH 값들
    //SHBuffer.SHCoefficients[0] = FVector4(0.5f, 0.5f, 0.5f, 0.0f);   // DC (base ambient)
    //SHBuffer.SHCoefficients[1] = FVector4(0.1f, 0.1f, 0.15f, 0.0f);  // +Y (sky/ceiling - blue tint)
    //SHBuffer.SHCoefficients[2] = FVector4(0.05f, 0.05f, 0.05f, 0.0f); // +Z
    //SHBuffer.SHCoefficients[3] = FVector4(0.2f, 0.15f, 0.1f, 0.0f);  // +X (warm side light)
    //SHBuffer.SHCoefficients[4] = FVector4(0.02f, 0.02f, 0.02f, 0.0f); // L2 bands (subtle directional)
    //SHBuffer.SHCoefficients[5] = FVector4(0.02f, 0.02f, 0.02f, 0.0f);
    //SHBuffer.SHCoefficients[6] = FVector4(0.03f, 0.03f, 0.03f, 0.0f);
    //SHBuffer.SHCoefficients[7] = FVector4(0.02f, 0.02f, 0.02f, 0.0f);
    //SHBuffer.SHCoefficients[8] = FVector4(0.02f, 0.02f, 0.02f, 0.0f);
    //SHBuffer.Intensity = SHIntensity;
    //return;

    // 아래 코드는 임시로 비활성화 (텍스처 읽기)
    // Zero out new SH coefficients
    for (int32 i = 0; i < 9; ++i)
    {
        SHBuffer.SHCoefficients[i] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // Read back each cubemap face and project to SH
    for (int32 FaceIdx = 0; FaceIdx < 6; ++FaceIdx)
    {
        ECubeFace Face = static_cast<ECubeFace>(FaceIdx);

        // Create staging texture for readback
        D3D11_TEXTURE2D_DESC stagingDesc;
        CubemapTextures[FaceIdx]->GetDesc(&stagingDesc);
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.MiscFlags = 0;

        ID3D11Texture2D* StagingTexture = nullptr;
        HRESULT hr = Renderer->GetRHIDevice()->GetDevice()->CreateTexture2D(&stagingDesc, nullptr, &StagingTexture);
        if (FAILED(hr))
            continue;

        // Copy to staging
        Context->CopyResource(StagingTexture, CubemapTextures[FaceIdx]);

        // Map and read
        D3D11_MAPPED_SUBRESOURCE mapped;
        hr = Context->Map(StagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            // R16G16B16A16_FLOAT uses half-precision floats
            uint16_t* PixelData = static_cast<uint16_t*>(mapped.pData);

            // Helper lambda: Convert IEEE 754 half-float to float
            auto HalfToFloat = [](uint16_t half) -> float
            {
                uint32_t sign = (half >> 15) & 0x1;
                uint32_t exponent = (half >> 10) & 0x1F;
                uint32_t mantissa = half & 0x3FF;

                if (exponent == 0)
                {
                    if (mantissa == 0) return sign ? -0.0f : 0.0f; // Zero
                    // Denormalized number
                    float result = mantissa / 1024.0f / 16384.0f;
                    return sign ? -result : result;
                }
                else if (exponent == 31)
                {
                    return (mantissa == 0) ? (sign ? -INFINITY : INFINITY) : NAN;
                }
                else
                {
                    // Normalized number
                    int32_t exp = exponent - 15 + 127; // Convert exponent from half to float bias
                    uint32_t floatBits = (sign << 31) | (exp << 23) | (mantissa << 13);
                    return *reinterpret_cast<float*>(&floatBits);
                }
            };

            // Process each pixel
            for (int32 y = 0; y < CaptureResolution; ++y)
            {
                for (int32 x = 0; x < CaptureResolution; ++x)
                {
                    // Get pixel color (R16G16B16A16_FLOAT)
                    int32 PixelIndex = y * (mapped.RowPitch / sizeof(uint16_t)) + x * 4;

                    // Convert half-float to float
                    FVector PixelColor;
                    PixelColor.X = HalfToFloat(PixelData[PixelIndex + 0]);
                    PixelColor.Y = HalfToFloat(PixelData[PixelIndex + 1]);
                    PixelColor.Z = HalfToFloat(PixelData[PixelIndex + 2]);

                    // Convert pixel position to direction
                    float U = (x + 0.5f) / CaptureResolution;
                    float V = (y + 0.5f) / CaptureResolution;
                    FVector Direction = CubeUVToDir(Face, U, V);
                    Direction.Normalize();

                    // Calculate solid angle for this texel
                    float SolidAngle = TexelSolidAngle(x, y, CaptureResolution);

                    // Accumulate SH coefficients (store as FVector4)
                    for (int32 i = 0; i < 9; ++i)
                    {
                        float Basis = SHBasis(i, Direction);
                                     //PixelColor=L(θ,ϕ)=방향별 빛의 색상(입사광 분포)
                                      //SolidAngle =sinθdθdϕ,각 픽셀이 차지하는 구면 면적
                        FVector Contribution = PixelColor * Basis * SolidAngle;
                        SHBuffer.SHCoefficients[i].X += Contribution.X;
                        SHBuffer.SHCoefficients[i].Y += Contribution.Y;
                        SHBuffer.SHCoefficients[i].Z += Contribution.Z;
                    }
                }
            }

            Context->Unmap(StagingTexture, 0);
        }

        StagingTexture->Release();
    }

    // Apply radiance to irradiance conversion (cosine convolution)
    // L0 band (DC) - multiply by π
    SHBuffer.SHCoefficients[0].X *= SH_A0;
    SHBuffer.SHCoefficients[0].Y *= SH_A0;
    SHBuffer.SHCoefficients[0].Z *= SH_A0;

    // L1 band (linear) - multiply by 2π/3
    for (int32 i = 1; i <= 3; ++i)
    {
        SHBuffer.SHCoefficients[i].X *= SH_A1;
        SHBuffer.SHCoefficients[i].Y *= SH_A1;
        SHBuffer.SHCoefficients[i].Z *= SH_A1;
    }

    // L2 band (quadratic) - multiply by π/4
    for (int32 i = 4; i <= 8; ++i)
    {
        SHBuffer.SHCoefficients[i].X *= SH_A2;
        SHBuffer.SHCoefficients[i].Y *= SH_A2;
        SHBuffer.SHCoefficients[i].Z *= SH_A2;
    }

    // Set intensity
    SHBuffer.Intensity = SHIntensity;
}

void UAmbientLightComponent::SmoothSH()
{
    // Temporal filtering: SH_smooth = lerp(SH_old, SH_new, SmoothingFactor)
    for (int32 i = 0; i < 9; ++i)
    {
        // Manual lerp: result = old + (new - old) * factor
        SHBuffer.SHCoefficients[i].X = SHBufferPrev.SHCoefficients[i].X +
            (SHBuffer.SHCoefficients[i].X - SHBufferPrev.SHCoefficients[i].X) * SmoothingFactor;
        SHBuffer.SHCoefficients[i].Y = SHBufferPrev.SHCoefficients[i].Y +
            (SHBuffer.SHCoefficients[i].Y - SHBufferPrev.SHCoefficients[i].Y) * SmoothingFactor;
        SHBuffer.SHCoefficients[i].Z = SHBufferPrev.SHCoefficients[i].Z +
            (SHBuffer.SHCoefficients[i].Z - SHBufferPrev.SHCoefficients[i].Z) * SmoothingFactor;
        SHBuffer.SHCoefficients[i].W = 0.0f; // W는 사용하지 않음
    }

    // ✅ Intensity도 smoothing (또는 그냥 유지)
    // Intensity는 사용자가 설정한 값을 그대로 유지
    SHBuffer.Intensity = SHIntensity;

    // Update previous for next frame
    SHBufferPrev = SHBuffer;
}

FMatrix UAmbientLightComponent::GetCubeFaceViewMatrix(ECubeFace Face) const
{
    FVector ProbePos = GetWorldLocation();
    FVector Forward, Up, Right;

    // DirectX Left-Handed 좌표계에서 큐브맵 각 면의 방향 직접 정의
    // 카메라 공간에서 각 축의 방향을 정의
    switch (Face)
    {
    case ECubeFace::PositiveX: // +X 면 (카메라가 +X를 바라봄)
        Right = FVector(0.0f, 0.0f, -1.0f);
        Up = FVector(0.0f, 1.0f, 0.0f);
        Forward = FVector(1.0f, 0.0f, 0.0f);
        break;
    case ECubeFace::NegativeX: // -X 면 (카메라가 -X를 바라봄)
        Right = FVector(0.0f, 0.0f, 1.0f);
        Up = FVector(0.0f, 1.0f, 0.0f);
        Forward = FVector(-1.0f, 0.0f, 0.0f);
        break;
    case ECubeFace::PositiveY: // +Y 면 (카메라가 +Y를 바라봄)
        Right = FVector(1.0f, 0.0f, 0.0f);
        Up = FVector(0.0f, 0.0f, -1.0f);
        Forward = FVector(0.0f, 1.0f, 0.0f);
        break;
    case ECubeFace::NegativeY: // -Y 면 (카메라가 -Y를 바라봄)
        Right = FVector(1.0f, 0.0f, 0.0f);
        Up = FVector(0.0f, 0.0f, 1.0f);
        Forward = FVector(0.0f, -1.0f, 0.0f);
        break;
    case ECubeFace::PositiveZ: // +Z 면 (카메라가 +Z를 바라봄)
        Right = FVector(1.0f, 0.0f, 0.0f);
        Up = FVector(0.0f, 1.0f, 0.0f);
        Forward = FVector(0.0f, 0.0f, 1.0f);
        break;
    case ECubeFace::NegativeZ: // -Z 면 (카메라가 -Z를 바라봄)
        Right = FVector(-1.0f, 0.0f, 0.0f);
        Up = FVector(0.0f, 1.0f, 0.0f);
        Forward = FVector(0.0f, 0.0f, -1.0f);
        break;
    }

    // View Matrix 직접 구성 (Left-Handed)
    // Row-major 형식
    FMatrix ViewMatrix;
    ViewMatrix.M[0][0] = Right.X;   ViewMatrix.M[0][1] = Up.X;   ViewMatrix.M[0][2] = Forward.X;   ViewMatrix.M[0][3] = 0.0f;
    ViewMatrix.M[1][0] = Right.Y;   ViewMatrix.M[1][1] = Up.Y;   ViewMatrix.M[1][2] = Forward.Y;   ViewMatrix.M[1][3] = 0.0f;
    ViewMatrix.M[2][0] = Right.Z;   ViewMatrix.M[2][1] = Up.Z;   ViewMatrix.M[2][2] = Forward.Z;   ViewMatrix.M[2][3] = 0.0f;
    ViewMatrix.M[3][0] = -FVector::Dot(Right, ProbePos);
    ViewMatrix.M[3][1] = -FVector::Dot(Up, ProbePos);
    ViewMatrix.M[3][2] = -FVector::Dot(Forward, ProbePos);
    ViewMatrix.M[3][3] = 1.0f;

    return ViewMatrix;
}

FMatrix UAmbientLightComponent::GetCubeFaceProjectionMatrix() const
{
    // 90 degree FOV, aspect ratio 1:1
    float FOV = 90.0f * (3.14159265358979323846f / 180.0f);  // Convert to radians
    float AspectRatio = 1.0f;
    float NearPlane = 0.1f;

    // Use max box extent as far plane if specified, otherwise use default
    float maxExtent = FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
    float FarPlane = (maxExtent > KINDA_SMALL_NUMBER) ? maxExtent : 1000.0f;

    return FMatrix::PerspectiveFovLH(FOV, AspectRatio, NearPlane, FarPlane);
}

FVector UAmbientLightComponent::CubeUVToDir(ECubeFace Face, float U, float V)
{
    // Convert UV [0,1] to [-1,1]
    // DirectX uses top-left origin for textures, so V needs to be flipped
    float S = U * 2.0f - 1.0f;
    float T = -(V * 2.0f - 1.0f);  // V 반전 (DirectX 텍스처 좌표계)

    FVector Dir;
    switch (Face)
    {
    case ECubeFace::PositiveX:
        Dir = FVector(1.0f, -S, -T);
        break;
    case ECubeFace::NegativeX:
        Dir = FVector(-1.0f, S, -T);
        break;
    case ECubeFace::PositiveY:
        Dir = FVector(S, 1.0f, T);
        break;
    case ECubeFace::NegativeY:
        Dir = FVector(S, -1.0f, -T);
        break;
    case ECubeFace::PositiveZ:
        Dir = FVector(S, -T, 1.0f);
        break;
    case ECubeFace::NegativeZ:
        Dir = FVector(-S, -T, -1.0f);
        break;
    }

    return Dir;
}

float UAmbientLightComponent::TexelSolidAngle(int32 X, int32 Y, int32 Size)
{
    // Accurate solid angle calculation for cubemap texel
    float InvSize = 1.0f / Size;
    float S = (X + 0.5f) * InvSize * 2.0f - 1.0f;
    float T = (Y + 0.5f) * InvSize * 2.0f - 1.0f;

    float x0 = S - InvSize;
    float x1 = S + InvSize;
    float y0 = T - InvSize;
    float y1 = T + InvSize;

    auto AreaElement = [](float x, float y) -> float
    {
        return atan2(x * y, sqrtf(x * x + y * y + 1.0f));
    };

    return AreaElement(x0, y0) - AreaElement(x0, y1) - AreaElement(x1, y0) + AreaElement(x1, y1);
}

float UAmbientLightComponent::SHBasis(int32 Index, const FVector& Dir)
{
    // Spherical harmonics basis functions (L2, 9 coefficients)
    // Using real SH basis functions

    const float PI = 3.14159265358979323846f;
    const float x = Dir.X;
    const float y = Dir.Y;
    const float z = Dir.Z;

    switch (Index)//K_l^m
    {
    // L0
    case 0: return 0.282095f;  // (1/2) * sqrt(1/π)

    // L1
    case 1: return 0.488603f * y;  // (1/2) * sqrt(3/π) * y
    case 2: return 0.488603f * z;  // (1/2) * sqrt(3/π) * z
    case 3: return 0.488603f * x;  // (1/2) * sqrt(3/π) * x

    // L2
    case 4: return 1.092548f * x * y;  // (1/2) * sqrt(15/π) * x * y
    case 5: return 1.092548f * y * z;  // (1/2) * sqrt(15/π) * y * z
    case 6: return 0.315392f * (3.0f * z * z - 1.0f);  // (1/4) * sqrt(5/π) * (3z² - 1)
    case 7: return 1.092548f * x * z;  // (1/2) * sqrt(15/π) * x * z
    case 8: return 0.546274f * (x * x - y * y);  // (1/4) * sqrt(15/π) * (x² - y²)

    default: return 0.0f;
    }
}

UObject* UAmbientLightComponent::Duplicate()
{
    UAmbientLightComponent* DuplicatedComponent = NewObject<UAmbientLightComponent>();
    CopyCommonProperties(DuplicatedComponent);
    DuplicatedComponent->AmbientData = this->AmbientData;
    DuplicatedComponent->BoxExtent = this->BoxExtent;
    DuplicatedComponent->Falloff = this->Falloff;
    DuplicatedComponent->CaptureResolution = this->CaptureResolution;
    DuplicatedComponent->UpdateInterval = this->UpdateInterval;
    DuplicatedComponent->SmoothingFactor = this->SmoothingFactor;
    DuplicatedComponent->SHIntensity = this->SHIntensity;
    DuplicatedComponent->DuplicateSubObjects();
    return DuplicatedComponent;
}

void UAmbientLightComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}

void UAmbientLightComponent::RenderDetails()
{
	ImGui::Separator();
	ImGui::Text("Ambient Light Component Settings");

	// SH Intensity
	float intensity = GetSHIntensity();
	if (ImGui::DragFloat("SH Intensity", &intensity, 0.01f, 0.0f, 10.0f))
	{
		SetSHIntensity(intensity);
	}

	ImGui::Spacing();

	// Localized Ambient Light Settings
	ImGui::Text("Localized Settings:");

	// Box Extent
	FVector extent = GetBoxExtent();
	if (ImGui::DragFloat3("Box Extent", &extent.X, 1.0f, 0.0f, 10000.0f))
	{
		SetBoxExtent(extent);
	}
	ImGui::SameLine();
	if (ImGui::Button("Global"))
	{
		SetBoxExtent(FVector(0.0f, 0.0f, 0.0f));  // 0 = global ambient light
	}

	// Uniform size slider (for convenience)
	float uniformSize = GetRadius();
	if (ImGui::DragFloat("Uniform Size", &uniformSize, 1.0f, 0.0f, 10000.0f))
	{
		SetRadius(uniformSize);
	}

	// Falloff
	float falloff = GetFalloff();
	if (ImGui::DragFloat("Falloff", &falloff, 0.05f, 0.1f, 10.0f))
	{
		SetFalloff(falloff);
	}

	ImGui::Spacing();

	// Capture Settings
	ImGui::Text("Capture Settings:");

	int32 resolution = GetCaptureResolution();
	if (ImGui::DragInt("Resolution", &resolution, 1, 16, 512))
	{
		SetCaptureResolution(resolution);
	}

	float updateInterval = GetUpdateInterval();
	if (ImGui::DragFloat("Update Interval", &updateInterval, 0.01f, 0.0f, 10.0f))
	{
		SetUpdateInterval(updateInterval);
	}

	float smoothing = GetSmoothingFactor();
	if (ImGui::DragFloat("Smoothing", &smoothing, 0.01f, 0.0f, 1.0f))
	{
		SetSmoothingFactor(smoothing);
	}

	ImGui::Spacing();

	// Force Capture Button
	if (ImGui::Button("Force Capture Now"))
	{
		ForceCapture();
	}
}

void UAmbientLightComponent::DrawDebugLines(class URenderer* Renderer)
{
	if (!Renderer || !IsRender())
		return;

	const FVector extent = GetBoxExtent();

	// If extent is 0, it's a global ambient light (no visible bounds)
	if (extent.SizeSquared() <= KINDA_SMALL_NUMBER)
		return;

	const FVector center = GetWorldLocation();
	const FVector4 color(0.0f, 1.0f, 1.0f, 1.0f); // Cyan for ambient light

	// Calculate 8 corners of the box
	FVector corners[8];
	corners[0] = center + FVector(-extent.X, -extent.Y, -extent.Z);
	corners[1] = center + FVector(+extent.X, -extent.Y, -extent.Z);
	corners[2] = center + FVector(+extent.X, +extent.Y, -extent.Z);
	corners[3] = center + FVector(-extent.X, +extent.Y, -extent.Z);
	corners[4] = center + FVector(-extent.X, -extent.Y, +extent.Z);
	corners[5] = center + FVector(+extent.X, -extent.Y, +extent.Z);
	corners[6] = center + FVector(+extent.X, +extent.Y, +extent.Z);
	corners[7] = center + FVector(-extent.X, +extent.Y, +extent.Z);

	// Draw bottom face (Z-)
	Renderer->AddLine(corners[0], corners[1], color);
	Renderer->AddLine(corners[1], corners[2], color);
	Renderer->AddLine(corners[2], corners[3], color);
	Renderer->AddLine(corners[3], corners[0], color);

	// Draw top face (Z+)
	Renderer->AddLine(corners[4], corners[5], color);
	Renderer->AddLine(corners[5], corners[6], color);
	Renderer->AddLine(corners[6], corners[7], color);
	Renderer->AddLine(corners[7], corners[4], color);

	// Draw vertical edges connecting top and bottom
	Renderer->AddLine(corners[0], corners[4], color);
	Renderer->AddLine(corners[1], corners[5], color);
	Renderer->AddLine(corners[2], corners[6], color);
	Renderer->AddLine(corners[3], corners[7], color);
}
