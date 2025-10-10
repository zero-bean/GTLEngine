#include "pch.h"
#include "Component/Public/DecalComponent.h"
#include "Texture/Public/Material.h" 
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UDecalComponent, UPrimitiveComponent)

UDecalComponent::UDecalComponent() : DecalMaterial(nullptr)
{
	if (DecalMaterial = NewObject<UMaterial>())
	{
		UAssetManager& AssetManager = UAssetManager::GetInstance();
		UTexture* DiffuseTexture = AssetManager.CreateTexture(FName("Asset/Texture/recovery_256x.png"), FName("recovery_256x"));
		DecalMaterial->SetDiffuseTexture(DiffuseTexture);
	}

	UAssetManager& ResourceManager = UAssetManager::GetInstance();

	Type = EPrimitiveType::CubeLine;
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);

	Indices = ResourceManager.GetIndexData(Type);
	IndexBuffer = ResourceManager.GetIndexbuffer(Type);
	NumIndices = ResourceManager.GetNumIndices(Type);

	BoundingBox = &ResourceManager.GetAABB(Type);

	// 렌더링 상태를 와이어프레임으로 설정 (DecalActor 생성자에서 가져옴)
	RenderState.FillMode = EFillMode::Solid;
	RenderState.CullMode = ECullMode::None;
}

UDecalComponent::~UDecalComponent()
{

}

void UDecalComponent::SetDecalMaterial(UMaterial* InMaterial)
{
	DecalMaterial = InMaterial;
}

UMaterial* UDecalComponent::GetDecalMaterial() const
{
	return DecalMaterial;
}
