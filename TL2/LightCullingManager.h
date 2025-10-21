#pragma once
#include <d3d11.h>
#include "Vector.h"

// Tile-based Light Culling Manager
// Manages compute shader execution and buffer resources for tiled light culling
class ULightCullingManager
{
public:
    ULightCullingManager();
    ~ULightCullingManager();

    // Initialize resources
    void Initialize(ID3D11Device* Device, UINT ScreenWidth, UINT ScreenHeight);

    // Execute light culling compute shader
    void ExecuteLightCulling(
        ID3D11DeviceContext* Context,
        ID3D11ShaderResourceView* DepthSRV,
        const FMatrix& ViewMatrix,
        const FMatrix& ProjMatrix,
        float NearPlane,
        float FarPlane,
        ID3D11Buffer* PointLightBuffer,
        ID3D11Buffer* SpotLightBuffer
    );

    // Bind results to pixel shader
    void BindResultsToPS(ID3D11DeviceContext* Context);

    // Unbind from pixel shader
    void UnbindFromPS(ID3D11DeviceContext* Context);

    // Resize (when screen resolution changes)
    void Resize(ID3D11Device* Device, UINT NewWidth, UINT NewHeight);

    // Release resources
    void Release();

    // Getters
    UINT GetNumTilesX() const { return NumTilesX; }
    UINT GetNumTilesY() const { return NumTilesY; }
    bool IsInitialized() const { return bInitialized; }

    // Debug visualization
    void SetDebugVisualization(bool bEnable) { bDebugVisualize = bEnable; }
    bool GetDebugVisualization() const { return bDebugVisualize; }

private:
    static constexpr UINT TILE_SIZE = 16;
    static constexpr UINT MAX_LIGHTS_PER_TILE = 256;

    // Screen dimensions
    UINT ScreenWidth = 0;
    UINT ScreenHeight = 0;
    UINT NumTilesX = 0;
    UINT NumTilesY = 0;
    UINT TotalTiles = 0;

    bool bInitialized = false;
    bool bDebugVisualize = false;

    // Compute Shader
    ID3D11ComputeShader* LightCullingCS = nullptr;
    ID3DBlob* CSBlob = nullptr;

    // Structured Buffers
    ID3D11Buffer* LightIndexListBuffer = nullptr;           // Output: flat list of light indices
    ID3D11Buffer* TileOffsetCountBuffer = nullptr;          // Output: per-tile offset and count

    ID3D11UnorderedAccessView* LightIndexListUAV = nullptr;
    ID3D11UnorderedAccessView* TileOffsetCountUAV = nullptr;

    ID3D11ShaderResourceView* LightIndexListSRV = nullptr;
    ID3D11ShaderResourceView* TileOffsetCountSRV = nullptr;

    // Constant Buffers
    ID3D11Buffer* CullingCB = nullptr;
    ID3D11Buffer* ViewMatrixCB = nullptr;
    ID3D11Buffer* TileCullingInfoCB = nullptr;

    // Helper methods
    void CreateComputeShader(ID3D11Device* Device);
    void CreateBuffers(ID3D11Device* Device);
    void ReleaseBuffers();

    // Constant buffer structures (must match HLSL)
    struct FCullingCBData
    {
        UINT ScreenWidth;
        UINT ScreenHeight;
        UINT NumTilesX;
        UINT NumTilesY;
        FVector2D NearFar;
        FVector2D Padding;
    };

    struct FViewMatrixCBData
    {
        FMatrix ViewMatrix;
        FMatrix ProjInv;
    };

    struct FTileCullingInfoCBData
    {
        UINT NumTilesX;
        UINT DebugVisualizeTiles;
        UINT Padding[2];
    };
};
