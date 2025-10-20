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

        // 각 라이팅 모델에 필요한 버텍스 셰이더를 생성합니다. 
        // 고로드 모델은 버텍스 셰이더를 별개로 필요로 하기 때문에, 일관성을 위해 배열로 만들었습니다.
        hr = D3DCompileFromFile(WFilePath.c_str(), GetMacros(currentModel), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", Flag, 0, &VSBlobs[i], &errorBlob);
        if (FAILED(hr))
        {
            char* msg = (char*)errorBlob->GetBufferPointer();
            UE_LOG("shader \'%s\' VS compile error for model %d: %s", InShaderPath, i, msg);
            if (errorBlob) errorBlob->Release();
            return; // Stop if any VS fails
        }

        hr = InDevice->CreateVertexShader(VSBlobs[i]->GetBufferPointer(), VSBlobs[i]->GetBufferSize(), nullptr, &VertexShaders[i]);
        if (FAILED(hr))
        {
            UE_LOG("CreateVertexShader failed for model %d", i);
            return;
        }

        // 각 라이팅 모델에 필요한 픽셀 셰이더를 생성합니다. 
        hr = D3DCompileFromFile(WFilePath.c_str(), GetMacros(currentModel), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", Flag, 0, &PSBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                char* msg = (char*)errorBlob->GetBufferPointer();
                UE_LOG("shader '%s' PS compile error for model %d: %s", InShaderPath, i, msg);
                errorBlob->Release();
            }
            return; // Stop if any PS fails
        }

        hr = InDevice->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, &PixelShaders[i]);
        if (FAILED(hr))
        {
            UE_LOG("CreatePixelShader failed for model %d", i);
            return;
        }
    }

    CreateInputLayout(InDevice, InShaderPath);
    
    ActiveModel = ELightShadingModel::BlinnPhong;
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

    for (int i = 0; i < (int)ELightShadingModel::Count; i++)
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

        if (PixelShaders[i])
        {
            PixelShaders[i]->Release();
            PixelShaders[i] = nullptr;
        }
    }
}
