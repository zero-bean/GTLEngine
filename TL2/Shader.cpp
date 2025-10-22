#include "pch.h"
#include "Shader.h"
 
static D3D_SHADER_MACRO MACRO_PHONG[] = {
    {"LIGHTING_MODEL_PHONG","1"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {"LIGHTING_MODEL_GOURAUD","0"},
    {"LIGHTING_MODEL_UNLIT","0"},
    {"USE_TILED_CULLING","1"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_BLINN[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","1"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {"LIGHTING_MODEL_GOURAUD","0"},
    {"LIGHTING_MODEL_UNLIT","0"},
    {"USE_TILED_CULLING","1"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_LAMBERT[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","1"},
    {"LIGHTING_MODEL_GOURAUD","0"},
    {"LIGHTING_MODEL_UNLIT","0"},
    {"USE_TILED_CULLING","1"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_BRDF[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","1"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {"LIGHTING_MODEL_GOURAUD","0"},
    {"LIGHTING_MODEL_UNLIT","0"},
    {"USE_TILED_CULLING","1"},
    {nullptr,nullptr}

};

static D3D_SHADER_MACRO MACRO_GOURAUD[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {"LIGHTING_MODEL_GOURAUD","1"},
    {"LIGHTING_MODEL_UNLIT","0"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_UNLIT[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {"LIGHTING_MODEL_GOURAUD","0"},
    {"LIGHTING_MODEL_UNLIT","1"},
    {"USE_TILED_CULLING","1"},
    {nullptr,nullptr}
};

void UShader::SetActiveNormalMode(ENormalMapMode InMode)
{
    if (ActiveNormalMode == InMode)
        return;

    ActiveNormalMode = InMode;

    auto key = std::make_pair(ActiveModel, InMode);
    if (CachedPS.count(key) == 0 || CachedVS.count(key) == 0)
    {
        ReloadForShadingModel(ActiveModel, Device);
        CachedPS[key] = PixelShaders;
        CachedVS[key] = VertexShaders;
    }
    else
    {
        // 캐시된 셰이더를 사용
        PixelShaders = CachedPS[key];
        VertexShaders = CachedVS[key];
    }
}

const D3D_SHADER_MACRO* UShader::GetMacros(ELightShadingModel Model)
{
    switch (Model)
    {
    case ELightShadingModel::Phong:
        return MACRO_PHONG;

    case ELightShadingModel::BlinnPhong:
        return MACRO_BLINN;

    case ELightShadingModel::BRDF:
        return MACRO_BRDF;

    case ELightShadingModel::Lambert:
        return MACRO_LAMBERT;

    case ELightShadingModel::Gouraud:
        return MACRO_GOURAUD;

    case ELightShadingModel::Unlit:
        return MACRO_UNLIT;

    default:
        return MACRO_BLINN;
    }
}

UShader::~UShader()
{
    ReleaseResources();
}

// 두 개의 셰이더 파일을 받는 주요 Load 함수
void UShader::Load(const FString& InShaderPath, ID3D11Device* InDevice)
{
    assert(InDevice);
    this->Device = InDevice;
    this->FilePath = InShaderPath;

    std::wstring WFilePath;  
    WFilePath = std::wstring(InShaderPath.begin(), InShaderPath.end());
       
    HRESULT hr;
    ID3DBlob* errorBlob = nullptr;
    UINT Flag = 0;
#ifdef _DEBUG
    Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif   
     
    // Non-uber shader: compile a single VS/PS without lighting-model permutations
    // Compile VS once
    hr = D3DCompileFromFile(WFilePath.c_str(), MACRO_BRDF, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", Flag, 0, &VSBlobs, &errorBlob);
    if (FAILED(hr))
    {
        char* msg = errorBlob ? (char*)errorBlob->GetBufferPointer() : (char*)"Unknown VS compile error";
        UE_LOG("shader '%s' VS compile error: %s", InShaderPath.c_str(), msg);
        if (errorBlob) { errorBlob->Release(); }
        return; 
    }

    ID3D11VertexShader* baseVS = nullptr;
    hr = InDevice->CreateVertexShader(VSBlobs->GetBufferPointer(), VSBlobs->GetBufferSize(), nullptr, &baseVS);
    if (FAILED(hr))
    {
        UE_LOG("CreateVertexShader failed (non-uber)");
        return;
    }

    // Assign compiled VS to the member (single variant path)
    VertexShaders = baseVS;

    // Compile PS once
    ID3DBlob* localPSBlob = nullptr;
    hr = D3DCompileFromFile(WFilePath.c_str(), MACRO_BRDF, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", Flag, 0, &localPSBlob, &errorBlob);
    if (FAILED(hr))
    {
        char* msg = errorBlob ? (char*)errorBlob->GetBufferPointer() : (char*)"Unknown PS compile error";
        UE_LOG("shader '%s' PS compile error: %s", InShaderPath.c_str(), msg);
        if (errorBlob) { errorBlob->Release(); }
        return;
    }

    ID3D11PixelShader* basePS = nullptr;
    hr = InDevice->CreatePixelShader(localPSBlob->GetBufferPointer(), localPSBlob->GetBufferSize(), nullptr, &basePS);
    if (localPSBlob) { localPSBlob->Release(); localPSBlob = nullptr; }
    if (FAILED(hr))
    {
        UE_LOG("CreatePixelShader failed (non-uber)");
        return;
    }

    this->PixelShaders = basePS;

    CreateInputLayout(InDevice, InShaderPath);

    ActiveModel = ELightShadingModel::BlinnPhong;
    ActiveNormalMode = ENormalMapMode::NoNormalMap;
}

void UShader::ReloadForShadingModel(ELightShadingModel Model, ID3D11Device* InDevice)
{
    assert(InDevice);
    this->Device = InDevice;

    // Release existing GPU resources before recompiling
    ReleaseResources();

    // Load Light UberShader
    std::wstring WFilePath = std::wstring(FilePath.begin(), FilePath.end());

    HRESULT hr;
    ID3DBlob* errorBlob = nullptr;
    UINT Flag = 0;
#ifdef _DEBUG
    Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    const D3D_SHADER_MACRO* baseMacros = GetMacros(Model);

    TArray<D3D_SHADER_MACRO> combinedMacros;
    if (baseMacros)
    {
        for (int i = 0; baseMacros[i].Name != nullptr; ++i)
        {
            combinedMacros.push_back(baseMacros[i]);
        }
    }

    if (this->ActiveNormalMode == ENormalMapMode::HasNormalMap)
    {
        combinedMacros.push_back({ "HAS_NORMAL_MAP", "1" });
    }
    else
    {
        combinedMacros.push_back({ "HAS_NORMAL_MAP", "0" });
    }

    combinedMacros.push_back({ nullptr, nullptr });

    // Compile VS/PS with the selected shading model macros
    hr = D3DCompileFromFile(WFilePath.c_str(), combinedMacros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", Flag, 0, &VSBlobs, &errorBlob);
    if (FAILED(hr))
    {
        char* msg = errorBlob ? (char*)errorBlob->GetBufferPointer() : (char*)"Unknown VS compile error";
        UE_LOG("[Shader Reload] VS compile error for '%s': %s", FilePath.c_str(), msg);
        if (errorBlob) { errorBlob->Release(); }
        return;
    }

    hr = InDevice->CreateVertexShader(VSBlobs->GetBufferPointer(), VSBlobs->GetBufferSize(), nullptr, &VertexShaders);
    if (FAILED(hr))
    {
        UE_LOG("[Shader Reload] CreateVertexShader failed for '%s'", FilePath.c_str());
        return;
    }

    ID3DBlob* localPSBlob = nullptr;
    hr = D3DCompileFromFile(WFilePath.c_str(), combinedMacros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", Flag, 0, &localPSBlob, &errorBlob);
    if (FAILED(hr))
    {
        char* msg = errorBlob ? (char*)errorBlob->GetBufferPointer() : (char*)"Unknown PS compile error";
        UE_LOG("[Shader Reload] PS compile error for '%s': %s", FilePath.c_str(), msg);
        if (errorBlob) { errorBlob->Release(); }
        return;
    }

    ID3D11PixelShader* recompiledPS = nullptr;
    hr = InDevice->CreatePixelShader(localPSBlob->GetBufferPointer(), localPSBlob->GetBufferSize(), nullptr, &recompiledPS);
    if (localPSBlob) { localPSBlob->Release(); localPSBlob = nullptr; }
    if (FAILED(hr))
    {
        UE_LOG("[Shader Reload] CreatePixelShader failed for '%s'", FilePath.c_str());
        return;
    }
     
    this->PixelShaders = recompiledPS; 

    CreateInputLayout(InDevice, FilePath);

    ActiveModel = Model;
}

void UShader::CreateInputLayout(ID3D11Device* Device, const FString& InShaderPath)
{
    TArray<D3D11_INPUT_ELEMENT_DESC> descArray = UResourceManager::GetInstance().GetProperInputLayout(InShaderPath);
    if (descArray.Num() == 0)
    {
        return;
    }
    const D3D11_INPUT_ELEMENT_DESC* layout = descArray.data();
    uint32 layoutCount = static_cast<uint32>(descArray.size());

    HRESULT hr = Device->CreateInputLayout(
        layout,
        layoutCount,
        VSBlobs->GetBufferPointer(), 
        VSBlobs->GetBufferSize(),
        &InputLayout);
    assert(SUCCEEDED(hr));
}

void UShader::ReleaseResources()
{
    if (PSBlob)
    {
        PSBlob->Release();
        PSBlob = nullptr;
    }

    if (InputLayout)
    {
        InputLayout->Release();
        InputLayout = nullptr;
    }

    if (VSBlobs)
    {
        VSBlobs->Release();
        VSBlobs = nullptr;
    }

    if (VertexShaders)
    {
        VertexShaders->Release();
        VertexShaders = nullptr;
    }

    if (PixelShaders)
    {
        PixelShaders->Release();
        PixelShaders = nullptr;
    }

    for (auto Ps : CachedPS)
    {
        Ps.second->Release();
        Ps.second = nullptr;
    }

    for (auto Vs : CachedVS)
    {
        Vs.second->Release();
        Vs.second = nullptr;
    }
}
