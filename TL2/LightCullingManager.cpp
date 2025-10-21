#include "pch.h"
#include "LightCullingManager.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

ULightCullingManager::ULightCullingManager()
{
}

ULightCullingManager::~ULightCullingManager()
{
    Release();
}

void ULightCullingManager::Initialize(ID3D11Device* Device, UINT InScreenWidth, UINT InScreenHeight)
{
    OutputDebugStringA("[LightCulling] Initialize() called\n");

    if (bInitialized)
    {
        OutputDebugStringA("[LightCulling] WARNING: Already initialized! Call Release() first or use Resize().\n");
        return;
    }

    ScreenWidth = InScreenWidth;
    ScreenHeight = InScreenHeight;

    // Calculate number of tiles
    NumTilesX = (ScreenWidth + TILE_SIZE - 1) / TILE_SIZE;
    NumTilesY = (ScreenHeight + TILE_SIZE - 1) / TILE_SIZE;
    TotalTiles = NumTilesX * NumTilesY;

    char msg[256];
    sprintf_s(msg, "[LightCulling] Screen: %dx%d, Tiles: %dx%d, Total: %d\n",
              ScreenWidth, ScreenHeight, NumTilesX, NumTilesY, TotalTiles);
    OutputDebugStringA(msg);

    // Create compute shader
    OutputDebugStringA("[LightCulling] Creating compute shader...\n");
    CreateComputeShader(Device);

    char csMsg[256];
    sprintf_s(csMsg, "[LightCulling] After CreateComputeShader - LightCullingCS: %p\n", LightCullingCS);
    OutputDebugStringA(csMsg);

    // Create buffers
    OutputDebugStringA("[LightCulling] Creating buffers...\n");
    CreateBuffers(Device);

    // IMPORTANT: Only mark as initialized if CS was successfully created
    if (LightCullingCS != nullptr)
    {
        bInitialized = true;
        sprintf_s(msg, "[LightCulling] Initialize complete - bInitialized: %d, CS: %p\n",
                  bInitialized, LightCullingCS);
        OutputDebugStringA(msg);
    }
    else
    {
        bInitialized = false;
        OutputDebugStringA("[LightCulling] ERROR: Initialize FAILED - Compute Shader is NULL!\n");
    }
}

void ULightCullingManager::CreateComputeShader(ID3D11Device* Device)
{
    HRESULT hr;

    OutputDebugStringA("[LightCulling] Compiling compute shader...\n");

    // Compile compute shader
    ID3DBlob* errorBlob = nullptr;

    // Use same path format as other shaders
    std::string shaderPathStr = "LightCulling.hlsl";
    std::wstring shaderPath = std::wstring(shaderPathStr.begin(), shaderPathStr.end());

    char pathMsg[1024];
    sprintf_s(pathMsg, "[LightCulling] Trying shader path: LightCulling.hlsl\n");
    OutputDebugStringA(pathMsg);

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    hr = D3DCompileFromFile(
        shaderPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "CS_LightCulling",
        "cs_5_0",
        compileFlags,
        0,
        &CSBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        char hrMsg[256];
        sprintf_s(hrMsg, "[LightCulling] FAILED to compile compute shader! HRESULT: 0x%08X\n", hr);
        OutputDebugStringA(hrMsg);

        if (errorBlob)
        {
            OutputDebugStringA("[LightCulling] === SHADER COMPILATION ERROR ===\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            OutputDebugStringA("[LightCulling] === END ERROR ===\n");
            errorBlob->Release();
        }
        else
        {
            OutputDebugStringA("[LightCulling] No error blob - file not found?\n");
        }
        return;
    }

    OutputDebugStringA("[LightCulling] Compute shader compiled successfully\n");

    // Create compute shader
    hr = Device->CreateComputeShader(
        CSBlob->GetBufferPointer(),
        CSBlob->GetBufferSize(),
        nullptr,
        &LightCullingCS
    );

    if (FAILED(hr))
    {
        char hrMsg2[256];
        sprintf_s(hrMsg2, "[LightCulling] FAILED to create compute shader object! HRESULT: 0x%08X\n", hr);
        OutputDebugStringA(hrMsg2);
    }
    else
    {
        char csMsg2[256];
        sprintf_s(csMsg2, "[LightCulling] Compute shader created successfully - LightCullingCS: %p\n", LightCullingCS);
        OutputDebugStringA(csMsg2);
    }
}

void ULightCullingManager::CreateBuffers(ID3D11Device* Device)
{
    HRESULT hr;

    // Calculate maximum possible light indices
    // Worst case: every tile has MAX_LIGHTS_PER_TILE lights
    UINT maxLightIndices = TotalTiles * MAX_LIGHTS_PER_TILE + 1; // +1 for counter

    // Create Light Index List Buffer (UAV + SRV)
    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = maxLightIndices * sizeof(UINT);
        bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bufferDesc.StructureByteStride = sizeof(UINT);

        hr = Device->CreateBuffer(&bufferDesc, nullptr, &LightIndexListBuffer);

        // Create UAV
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = maxLightIndices;
        uavDesc.Buffer.Flags = 0;

        hr = Device->CreateUnorderedAccessView(LightIndexListBuffer, &uavDesc, &LightIndexListUAV);

        // Create SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = maxLightIndices;

        hr = Device->CreateShaderResourceView(LightIndexListBuffer, &srvDesc, &LightIndexListSRV);
    }

    // Create Tile Offset/Count Buffer (UAV + SRV)
    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = TotalTiles * sizeof(UINT) * 2; // uint2 per tile
        bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bufferDesc.StructureByteStride = sizeof(UINT) * 2;

        hr = Device->CreateBuffer(&bufferDesc, nullptr, &TileOffsetCountBuffer);

        // Create UAV
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = TotalTiles;
        uavDesc.Buffer.Flags = 0;

        hr = Device->CreateUnorderedAccessView(TileOffsetCountBuffer, &uavDesc, &TileOffsetCountUAV);

        // Create SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = TotalTiles;

        hr = Device->CreateShaderResourceView(TileOffsetCountBuffer, &srvDesc, &TileOffsetCountSRV);
    }

    // Create Constant Buffers
    {
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // CullingCB
        cbDesc.ByteWidth = sizeof(FCullingCBData);
        hr = Device->CreateBuffer(&cbDesc, nullptr, &CullingCB);

        // ViewMatrixCB
        cbDesc.ByteWidth = sizeof(FViewMatrixCBData);
        hr = Device->CreateBuffer(&cbDesc, nullptr, &ViewMatrixCB);

        // TileCullingInfoCB
        cbDesc.ByteWidth = sizeof(FTileCullingInfoCBData);
        hr = Device->CreateBuffer(&cbDesc, nullptr, &TileCullingInfoCB);
    }
}

void ULightCullingManager::ExecuteLightCulling(
    ID3D11DeviceContext* Context,
    ID3D11ShaderResourceView* DepthSRV,
    const FMatrix& ViewMatrix,
    const FMatrix& ProjMatrix,
    float NearPlane,
    float FarPlane,
    ID3D11Buffer* PointLightBuffer,
    ID3D11Buffer* SpotLightBuffer)
{
    char statusMsg[256];
    sprintf_s(statusMsg, "[LightCulling] ExecuteLightCulling - bInitialized: %d, LightCullingCS: %p\n",
              bInitialized, LightCullingCS);
    OutputDebugStringA(statusMsg);

    if (!bInitialized || !LightCullingCS)
    {
        OutputDebugStringA("[LightCulling] NOT INITIALIZED or CS is NULL!\n");
        return;
    }

    if (!PointLightBuffer || !SpotLightBuffer)
    {
        OutputDebugStringA("[LightCulling] WARNING: Light buffers are NULL!\n");
        return;
    }

    char execMsg[256];
    sprintf_s(execMsg, "[LightCulling] Executing light culling... Near=%.2f, Far=%.2f\n", NearPlane, FarPlane);
    OutputDebugStringA(execMsg);

    // Clear the light index counter (first element of LightIndexListBuffer)
    UINT clearValue[4] = { 0, 0, 0, 0 };
    Context->ClearUnorderedAccessViewUint(LightIndexListUAV, clearValue);

    // Update constant buffers
    D3D11_MAPPED_SUBRESOURCE mapped;

    // Update CullingCB
    {
        Context->Map(CullingCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        FCullingCBData* data = (FCullingCBData*)mapped.pData;
        data->ScreenWidth = ScreenWidth;
        data->ScreenHeight = ScreenHeight;
        data->NumTilesX = NumTilesX;
        data->NumTilesY = NumTilesY;
        data->NearFar = FVector2D(NearPlane, FarPlane);
        data->Padding = FVector2D(0, 0);
        Context->Unmap(CullingCB, 0);
    }

    // Update ViewMatrixCB with ProjInv
    {
        Context->Map(ViewMatrixCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        FViewMatrixCBData* data = (FViewMatrixCBData*)mapped.pData;
        data->ViewMatrix = ViewMatrix;

        // Calculate Projection Inverse matrix
        FMatrix ProjInv = ProjMatrix.Inverse();
        data->ProjInv = ProjInv;

        Context->Unmap(ViewMatrixCB, 0);
    }

    // Set compute shader
    Context->CSSetShader(LightCullingCS, nullptr, 0);

    // Set constant buffers
    Context->CSSetConstantBuffers(0, 1, &CullingCB);
    Context->CSSetConstantBuffers(1, 1, &ViewMatrixCB);

    // Bind light buffers directly (passed from Renderer)
    // PointLight at b9, SpotLight at b13
    OutputDebugStringA("[LightCulling] Binding light buffers (b9, b13) to Compute Shader\n");
    Context->CSSetConstantBuffers(9, 1, &PointLightBuffer);
    Context->CSSetConstantBuffers(13, 1, &SpotLightBuffer);

    // Set depth texture (SRV)
    Context->CSSetShaderResources(0, 1, &DepthSRV);

    // Set UAVs (output buffers)
    Context->CSSetUnorderedAccessViews(0, 1, &LightIndexListUAV, nullptr);
    Context->CSSetUnorderedAccessViews(1, 1, &TileOffsetCountUAV, nullptr);



    // Dispatch compute shader
    char msg[256];
    sprintf_s(msg, "[LightCulling] Dispatching: %d x %d tiles\n", NumTilesX, NumTilesY);
    OutputDebugStringA(msg);

    Context->Dispatch(NumTilesX, NumTilesY, 1);

    OutputDebugStringA("[LightCulling] Dispatch complete\n");

    // DEBUG: Read back first few tiles to verify data
    {
        D3D11_BUFFER_DESC desc;
        TileOffsetCountBuffer->GetDesc(&desc);

        // Create staging buffer for readback
        D3D11_BUFFER_DESC stagingDesc = desc;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.MiscFlags = 0;

        ID3D11Device* device;
        Context->GetDevice(&device);

        ID3D11Buffer* stagingBuffer = nullptr;
        HRESULT hr = device->CreateBuffer(&stagingDesc, nullptr, &stagingBuffer);

        if (SUCCEEDED(hr) && stagingBuffer)
        {
            // Copy GPU data to staging buffer
            Context->CopyResource(stagingBuffer, TileOffsetCountBuffer);

            // Map and read
            D3D11_MAPPED_SUBRESOURCE mapped;
            hr = Context->Map(stagingBuffer, 0, D3D11_MAP_READ, 0, &mapped);

            if (SUCCEEDED(hr))
            {
                UINT* data = (UINT*)mapped.pData;

                // Count how many tiles have lights
                int tilesWithLights = 0;
                int maxLightsInTile = 0;
                for (UINT i = 0; i < FMath::Min(TotalTiles, 100u); ++i)
                {
                    UINT lightCount = data[i * 2 + 1]; // Second element is count
                    if (lightCount > 0)
                    {
                        tilesWithLights++;
                        maxLightsInTile = FMath::Max((int)lightCount, maxLightsInTile);
                    }
                }

                char debugMsg[512];
                sprintf_s(debugMsg, "[LightCulling] Tiles with lights: %d/%d, Max lights in a tile: %d\n",
                          tilesWithLights, FMath::Min(TotalTiles, 100u), maxLightsInTile);
                OutputDebugStringA(debugMsg);

                sprintf_s(debugMsg, "[LightCulling] First 5 tiles: [0]=(%u,%u) [1]=(%u,%u) [2]=(%u,%u) [3]=(%u,%u) [4]=(%u,%u)\n",
                          data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);
                OutputDebugStringA(debugMsg);

                Context->Unmap(stagingBuffer, 0);
            }

            stagingBuffer->Release();
        }

        device->Release();
    }

    // Unbind UAVs
    ID3D11UnorderedAccessView* nullUAV[2] = { nullptr, nullptr };
    Context->CSSetUnorderedAccessViews(0, 2, nullUAV, nullptr);

    // ✅ 추가: UAV → SRV 전환을 보장
    Context->Flush();
    // Unbind SRVs
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    Context->CSSetShaderResources(0, 1, nullSRV);

    // Unbind compute shader
    Context->CSSetShader(nullptr, nullptr, 0);

 

}

void ULightCullingManager::BindResultsToPS(ID3D11DeviceContext* Context)
{
    if (!bInitialized)
        return;

    char msg[256];
    sprintf_s(msg, "[LightCulling] Binding to PS - NumTilesX: %d, Debug: %d\n",
              NumTilesX, bDebugVisualize ? 1 : 0);
    OutputDebugStringA(msg);

    // Bind SRVs to pixel shader (t10, t11)
    Context->PSSetShaderResources(10, 1, &LightIndexListSRV);
    Context->PSSetShaderResources(11, 1, &TileOffsetCountSRV);

    // Update and bind TileCullingInfoCB (b12)
    D3D11_MAPPED_SUBRESOURCE mapped;
    Context->Map(TileCullingInfoCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    FTileCullingInfoCBData* data = (FTileCullingInfoCBData*)mapped.pData;
    data->NumTilesX = NumTilesX;
    data->DebugVisualizeTiles = bDebugVisualize ? 1 : 0;
    data->Padding[0] = data->Padding[1] = 0;
    Context->Unmap(TileCullingInfoCB, 0);

    Context->PSSetConstantBuffers(6, 1, &TileCullingInfoCB);
}

void ULightCullingManager::UnbindFromPS(ID3D11DeviceContext* Context)
{
    ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
    Context->PSSetShaderResources(10, 2, nullSRV);
}

void ULightCullingManager::Resize(ID3D11Device* Device, UINT NewWidth, UINT NewHeight)
{
    if (NewWidth == ScreenWidth && NewHeight == ScreenHeight)
        return;

    char resizeMsg[256];
    sprintf_s(resizeMsg, "[LightCulling] Resize from %dx%d to %dx%d\n",
              ScreenWidth, ScreenHeight, NewWidth, NewHeight);
    OutputDebugStringA(resizeMsg);

    // Release buffers (but keep compute shader and blob)
    ReleaseBuffers();

    // Update dimensions
    ScreenWidth = NewWidth;
    ScreenHeight = NewHeight;

    // Recalculate tiles
    NumTilesX = (ScreenWidth + TILE_SIZE - 1) / TILE_SIZE;
    NumTilesY = (ScreenHeight + TILE_SIZE - 1) / TILE_SIZE;
    TotalTiles = NumTilesX * NumTilesY;

    sprintf_s(resizeMsg, "[LightCulling] New tiles: %dx%d = %d total\n",
              NumTilesX, NumTilesY, TotalTiles);
    OutputDebugStringA(resizeMsg);

    // Recreate buffers with new size
    CreateBuffers(Device);

    OutputDebugStringA("[LightCulling] Resize complete\n");
}

void ULightCullingManager::ReleaseBuffers()
{
    if (LightIndexListBuffer) { LightIndexListBuffer->Release(); LightIndexListBuffer = nullptr; }
    if (TileOffsetCountBuffer) { TileOffsetCountBuffer->Release(); TileOffsetCountBuffer = nullptr; }
    if (LightIndexListUAV) { LightIndexListUAV->Release(); LightIndexListUAV = nullptr; }
    if (TileOffsetCountUAV) { TileOffsetCountUAV->Release(); TileOffsetCountUAV = nullptr; }
    if (LightIndexListSRV) { LightIndexListSRV->Release(); LightIndexListSRV = nullptr; }
    if (TileOffsetCountSRV) { TileOffsetCountSRV->Release(); TileOffsetCountSRV = nullptr; }
    if (CullingCB) { CullingCB->Release(); CullingCB = nullptr; }
    if (ViewMatrixCB) { ViewMatrixCB->Release(); ViewMatrixCB = nullptr; }
    if (TileCullingInfoCB) { TileCullingInfoCB->Release(); TileCullingInfoCB = nullptr; }
}

void ULightCullingManager::Release()
{
    ReleaseBuffers();

    if (LightCullingCS) { LightCullingCS->Release(); LightCullingCS = nullptr; }
    if (CSBlob) { CSBlob->Release(); CSBlob = nullptr; }

    bInitialized = false;
}
