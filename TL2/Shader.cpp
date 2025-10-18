#include "pch.h"
#include "Shader.h"
 
static D3D_SHADER_MACRO MACRO_PHONG[] = {
    {"LIGHTING_MODEL_PHONG","1"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_BLINN[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","1"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","0"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_LAMBERT[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","0"},
    {"LIGHTING_MODEL_LAMBERT","1"},
    {nullptr,nullptr}
};

static D3D_SHADER_MACRO MACRO_BRDF[] = {
    {"LIGHTING_MODEL_PHONG","0"},
    {"LIGHTING_MODEL_BLINN_PHONG","0"},
    {"LIGHTING_MODEL_BRDF","1"},
    {"LIGHTING_MODEL_LAMBERT","0"},
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

    hr = D3DCompileFromFile(WFilePath.c_str(), DefinesRender, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", Flag, 0, &VSBlob, &errorBlob);
    if (FAILED(hr))
    {
        char* msg = (char*)errorBlob->GetBufferPointer();
        UE_LOG("shader \'%s\'compile error: %s", InShaderPath, msg);
        if (errorBlob) errorBlob->Release();
        return;
    }

    hr = InDevice->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr, &VertexShader);

    for (int i = 0; i < (int)ELightShadingModel::Count; ++i)
    { 
        hr = D3DCompileFromFile(WFilePath.c_str(), GetMacros(ELightShadingModel(i)), D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", Flag, 0, &PSBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                char* msg = (char*)errorBlob->GetBufferPointer();
                UE_LOG("shader '%s' PS compile error: %s", InShaderPath, msg);
                errorBlob->Release();
            }
            return;
        }

        hr = InDevice->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, &PixelShaders[(int)ELightShadingModel(i)]);
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
        VSBlob->GetBufferPointer(), 
        VSBlob->GetBufferSize(),
        &InputLayout);
    assert(SUCCEEDED(hr));
}

void UShader::ReleaseResources()
{
    if (VSBlob)
    {
        VSBlob->Release();
        VSBlob = nullptr;
    }
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
    if (VertexShader)
    {
        VertexShader->Release();
        VertexShader = nullptr;
    }
    if (PixelShaders)
    {
        for (int i = 0; i < (int)ELightShadingModel::Count ; i++)
        {
            PixelShaders[i]->Release();
            PixelShaders[i] = nullptr;
        }
    }
     
}
