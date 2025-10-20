#include "pch.h"
#include "Shader.h"
 
static D3D_SHADER_MACRO MACRO_PHONG[] = {
    {"LIGHTING_MODEL_PHONG","1"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {"LIGHTING_MODEL_GOURAUD","0"},
    {"LIGHTING_MODEL_UNLIT","0"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_BLINN[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","1"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {"LIGHTING_MODEL_GOURAUD","0"},
    {"LIGHTING_MODEL_UNLIT","0"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_LAMBERT[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","1"},
    {"LIGHTING_MODEL_GOURAUD","0"},
    {"LIGHTING_MODEL_UNLIT","0"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_BRDF[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","1"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {"LIGHTING_MODEL_GOURAUD","0"},
    {"LIGHTING_MODEL_UNLIT","0"},
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

    std::wstring WFilePath;
    WFilePath = std::wstring(InShaderPath.begin(), InShaderPath.end());

    HRESULT hr;
    ID3DBlob* errorBlob = nullptr;
    UINT Flag = 0;
#ifdef _DEBUG
    Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif 

    for (int i = 0; i < (int)ELightShadingModel::Count; ++i)
    {
        ELightShadingModel currentModel = ELightShadingModel(i);
        const D3D_SHADER_MACRO* baseMacros = GetMacros(currentModel);

        // ⭐ VS는 노멀맵과 무관하게 라이팅 모델당 1개만 생성
        hr = D3DCompileFromFile(WFilePath.c_str(), baseMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", Flag, 0, &VSBlobs[i], &errorBlob);

        if (FAILED(hr))
        {
            char* msg = (char*)errorBlob->GetBufferPointer();
            UE_LOG("shader '%s' VS compile error for model %d: %s", InShaderPath.c_str(), i, msg);
            if (errorBlob) errorBlob->Release();
            return;
        }

        hr = InDevice->CreateVertexShader(VSBlobs[i]->GetBufferPointer(), VSBlobs[i]->GetBufferSize(), nullptr, &VertexShaders[i]);

        if (FAILED(hr))
        {
            UE_LOG("CreateVertexShader failed for model %d", i);
            return;
        }

        // PS는 라이팅 모델 × 노멀맵 모드 조합으로 생성
        for (int j = 0; j < (int)ENormalMapMode::ENormalMapModeCount; ++j)
        {
            std::vector<D3D_SHADER_MACRO> allMacros;

            // 라이팅 모델 매크로 추가
            for (int m = 0; baseMacros[m].Name != nullptr; ++m)
            {
                allMacros.push_back(baseMacros[m]);
            }

            // 노멀맵 매크로 추가
            allMacros.push_back({ "HAS_NORMAL_MAP", (j == (int)ENormalMapMode::HasNormalMap) ? "1" : "0" });
            allMacros.push_back({ nullptr, nullptr });

            ID3DBlob* currentPSBlob = nullptr;
            hr = D3DCompileFromFile(WFilePath.c_str(), allMacros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", Flag, 0, &currentPSBlob, &errorBlob);

            if (FAILED(hr))
            {
                char* msg = (char*)errorBlob->GetBufferPointer();
                UE_LOG("shader '%s' PS compile error for model %d, normalMode %d: %s", InShaderPath.c_str(), i, j, msg);
                if (errorBlob) { errorBlob->Release(); }
                return;
            }

            hr = InDevice->CreatePixelShader(currentPSBlob->GetBufferPointer(), currentPSBlob->GetBufferSize(), nullptr, &PixelShaders[i][j]);

            currentPSBlob->Release();

            if (FAILED(hr))
            {
                UE_LOG("CreatePixelShader failed for model %d, normalMode %d", i, j);
                return;
            }
        }
    }

    CreateInputLayout(InDevice, InShaderPath);

    ActiveModel = ELightShadingModel::BlinnPhong;
    ActiveNormalMode = ENormalMapMode::NoNormalMap;
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
        VSBlobs[0]->GetBufferPointer(), 
        VSBlobs[0]->GetBufferSize(),
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

    for (int i = 0; i < (int)ELightShadingModel::Count; ++i)
    {
        if (VSBlobs[i])
        {
            VSBlobs[i]->Release();
            VSBlobs[i] = nullptr;
        }

        if (VertexShaders[i])
        {
            VertexShaders[i]->Release();
            VertexShaders[i] = nullptr;
        }

        for (int j = 0; j < (int)ENormalMapMode::ENormalMapModeCount; ++j)
        {
            if (PixelShaders[i][j])
            {
                PixelShaders[i][j]->Release();
                PixelShaders[i][j] = nullptr;
            }
        }

    }
}
