#include "stdafx.h"
#include "UMaterialManager.h"
#include "UClass.h"
#include "UTexture.h"
#include "UMaterial.h"
#include "URenderer.h"

IMPLEMENT_UCLASS(UMaterialManager, UEngineSubsystem)

UMaterialManager::UMaterialManager()
{

}

UMaterialManager::~UMaterialManager()
{
    for (auto& pair : Materials) { SAFE_DELETE(pair.second); }
    Materials.clear();
}

bool UMaterialManager::Initialize(URenderer* renderer)
{
    if (!renderer) { return false; }

    // 텍스처 X, Vertex + Color 조합
    {
        UMaterial* Material = NewObject<UMaterial>();
        Material->SetShader(renderer->GetVertexShader("Default"),
            renderer->GetPixelShader("Default"),
            renderer->GetInputLayout("Default"));
        Materials["Default"] = Material;
    }

    // 텍스처 X, Vertex + Color 조합, 라인리스트 전용
    {
        UMaterial* Material = NewObject<UMaterial>();
        Material->SetShader(renderer->GetVertexShader("Line"),
            renderer->GetPixelShader("Line"),
            renderer->GetInputLayout("Line"));
        Materials["Line"] = Material;
    }

    // 렌 이미지를 적용한 Material
    {
        // 텍스쳐 로드
        UTexture* Texture = NewObject<UTexture>();
        Texture->LoadFromFile(renderer->GetDevice(), L"assets/rabbit.dds");
        // 머티리얼 생성
        UMaterial* Material = NewObject<UMaterial>();
        Material->SetShader(renderer->GetVertexShader("Image"),
            renderer->GetPixelShader("Image"),
            renderer->GetInputLayout("Image"));
        Material->SetTexture(Texture);

        Materials["Image"] = Material;
    }

    // 폰트 이미지를 적용한 Material
    {
        // 텍스쳐 로드
        UTexture* Texture = NewObject<UTexture>();
        Texture->LoadFromFile(renderer->GetDevice(), L"assets/font_transparent.dds");
        // 머티리얼 생성
        UMaterial* Material = NewObject<UMaterial>();
        Material->SetShader(renderer->GetVertexShader("Font"),
            renderer->GetPixelShader("Font"),
            renderer->GetInputLayout("Font"));
        Material->SetTexture(Texture);
        Materials["Font"] = Material;
    }

    return true;
}

UMaterial* UMaterialManager::RetrieveMaterial(const FString& name)
{
    auto it = Materials.find(name);
    if (it == Materials.end()) { return nullptr; }

    return it->second;
}
