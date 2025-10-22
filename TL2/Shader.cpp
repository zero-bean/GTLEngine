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

    // [FIX] Check if this is the UberShader
    bool bIsUberShader = (InShaderPath.find("UberLit.hlsl") != std::string::npos);

    std::wstring WFilePath = std::wstring(InShaderPath.begin(), InShaderPath.end());

    HRESULT hr;
    ID3DBlob* errorBlob = nullptr;
    UINT Flag = 0;
#ifdef _DEBUG
    Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif   

    // --- Compile VS ---
    // Use the default shading model's macros for the VS
    const D3D_SHADER_MACRO* defaultVSMacros = GetMacros(ActiveModel); // ActiveModel defaults to BlinnPhong

    TArray<D3D_SHADER_MACRO> vsMacros;
    if (defaultVSMacros)
    {
        for (int i = 0; defaultVSMacros[i].Name != nullptr; ++i)
        {
            vsMacros.push_back(defaultVSMacros[i]);
        }
    }
    vsMacros.push_back({ "HAS_NORMAL_MAP", "0" }); // VS only needs one compile
    vsMacros.push_back({ nullptr, nullptr });

    hr = D3DCompileFromFile(WFilePath.c_str(), vsMacros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", Flag, 0, &VSBlobs, &errorBlob);
    if (FAILED(hr))
    {
        char* msg = errorBlob ? (char*)errorBlob->GetBufferPointer() : (char*)"Unknown VS compile error";
        UE_LOG("shader '%s' VS compile error: %s", InShaderPath.c_str(), msg);
        if (errorBlob) { errorBlob->Release(); }
        return;
    }

    hr = InDevice->CreateVertexShader(VSBlobs->GetBufferPointer(), VSBlobs->GetBufferSize(), nullptr, &VertexShaders);
    if (FAILED(hr))
    {
        UE_LOG("CreateVertexShader failed for '%s'", InShaderPath.c_str());
        if (VSBlobs) { VSBlobs->Release(); VSBlobs = nullptr; }
        return;
    }

    // --- Compile PS ---
    if (bIsUberShader)
    {
        // [FIX] It's the UberShader, compile all PS variants (just like ReloadForShadingModel)
        UE_LOG("Loading UberShader '%s', compiling all variants...", InShaderPath.c_str());
        const D3D_SHADER_MACRO* baseMacros = GetMacros(ActiveModel); // Use default model

        for (int i = 0; i < (int)ENormalMapMode::ENormalMapModeCount; ++i)
        {
            ENormalMapMode currentVariant = (ENormalMapMode)i;
            TArray<D3D_SHADER_MACRO> combinedMacros;
            if (baseMacros)
            {
                for (int j = 0; baseMacros[j].Name != nullptr; ++j)
                {
                    combinedMacros.push_back(baseMacros[j]);
                }
            }

            if (currentVariant == ENormalMapMode::HasNormalMap)
            {
                combinedMacros.push_back({ "HAS_NORMAL_MAP", "1" });
            }
            else
            {
                combinedMacros.push_back({ "HAS_NORMAL_MAP", "0" });
            }
            combinedMacros.push_back({ nullptr, nullptr });

            // Compile PS
            ID3DBlob* localPSBlob = nullptr;
            hr = D3DCompileFromFile(WFilePath.c_str(), combinedMacros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", Flag, 0, &localPSBlob, &errorBlob);
            if (FAILED(hr))
            {
                char* msg = errorBlob ? (char*)errorBlob->GetBufferPointer() : (char*)"Unknown PS compile error";
                UE_LOG("[Shader Load] PS compile error for '%s' (Variant %d): %s", InShaderPath.c_str(), i, msg);
                if (errorBlob) { errorBlob->Release(); }
                ReleaseResources();
                return;
            }

            // Create PS and store in array
            hr = InDevice->CreatePixelShader(localPSBlob->GetBufferPointer(), localPSBlob->GetBufferSize(), nullptr, &this->PixelShaders[i]);
            if (localPSBlob) { localPSBlob->Release(); localPSBlob = nullptr; }
            if (FAILED(hr))
            {
                UE_LOG("[Shader Load] CreatePixelShader failed for '%s' (Variant %d)", InShaderPath.c_str(), i);
                ReleaseResources();
                return;
            }
        } // --- End of PS variant loop ---
    }
    else
    {
        // [EXISTING] Non-uber shader: compile a single PS (NoNormalMap variant)
        UE_LOG("Loading non-uber shader '%s', compiling single variant...", InShaderPath.c_str());
        ID3DBlob* localPSBlob = nullptr;
        // Use vsMacros (which has HAS_NORMAL_MAP 0)
        hr = D3DCompileFromFile(WFilePath.c_str(), vsMacros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", Flag, 0, &localPSBlob, &errorBlob);
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

        this->PixelShaders[(size_t)ENormalMapMode::NoNormalMap] = basePS;
        this->PixelShaders[(size_t)ENormalMapMode::HasNormalMap] = nullptr;
    }

    CreateInputLayout(InDevice, InShaderPath);
    // ActiveModel is already set to default (BlinnPhong)
    ActiveNormalMode = ENormalMapMode::NoNormalMap; // Default to this
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

    TArray<D3D_SHADER_MACRO> vsMacros;
    if (baseMacros)
    {
        for (int i = 0; baseMacros[i].Name != nullptr; ++i)
        {
            vsMacros.push_back(baseMacros[i]);
        }
    }
    vsMacros.push_back({ "HAS_NORMAL_MAP", "0" });
    vsMacros.push_back({ nullptr, nullptr });

    // Compile VS
    hr = D3DCompileFromFile(WFilePath.c_str(), vsMacros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", Flag, 0, &VSBlobs, &errorBlob);
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
        if (VSBlobs) { VSBlobs->Release(); VSBlobs = nullptr; }
        return;
    }

    for (int i = 0; i < (int)ENormalMapMode::ENormalMapModeCount; ++i)
    {
        ENormalMapMode currentVariant = (ENormalMapMode)i;

        TArray<D3D_SHADER_MACRO> combinedMacros;
        if (baseMacros)
        {
            for (int j = 0; baseMacros[j].Name != nullptr; ++j)
            {
                combinedMacros.push_back(baseMacros[j]);
            }
        }

        if (currentVariant == ENormalMapMode::HasNormalMap)
        {
            combinedMacros.push_back({ "HAS_NORMAL_MAP", "1" });
        }
        else
        {
            combinedMacros.push_back({ "HAS_NORMAL_MAP", "0" });
        }
        combinedMacros.push_back({ nullptr, nullptr });

        // Compile PS
        ID3DBlob* localPSBlob = nullptr;
        hr = D3DCompileFromFile(WFilePath.c_str(), combinedMacros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", Flag, 0, &localPSBlob, &errorBlob);
        if (FAILED(hr))
        {
            char* msg = errorBlob ? (char*)errorBlob->GetBufferPointer() : (char*)"Unknown PS compile error";
            UE_LOG("[Shader Reload] PS compile error for '%s' (Variant %d): %s", FilePath.c_str(), i, msg);
            if (errorBlob) { errorBlob->Release(); }
            ReleaseResources();
            return;
        }

        // Create PS and store in array
        hr = InDevice->CreatePixelShader(localPSBlob->GetBufferPointer(), localPSBlob->GetBufferSize(), nullptr, &this->PixelShaders[i]);
        if (localPSBlob) { localPSBlob->Release(); localPSBlob = nullptr; }
        if (FAILED(hr))
        {
            UE_LOG("[Shader Reload] CreatePixelShader failed for '%s' (Variant %d)", FilePath.c_str(), i);
            ReleaseResources();
            return;
        }
    } 

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

    if (!VSBlobs)
    {
        UE_LOG("CreateInputLayout failed: VSBlob is null.");
        return;
    }

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

    for (int i = 0; i < (int)ENormalMapMode::ENormalMapModeCount; ++i)
    {
        if (PixelShaders[i])
        {
            PixelShaders[i]->Release();
            PixelShaders[i] = nullptr;
        }
    }
}
