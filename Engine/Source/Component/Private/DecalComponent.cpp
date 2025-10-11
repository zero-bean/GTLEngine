#include "pch.h"
#include "Component/Public/DecalComponent.h"
#include "Texture/Public/Material.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UDecalComponent, UPrimitiveComponent)

UDecalComponent::UDecalComponent() : DecalMaterial(nullptr)
{
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	UAssetManager& ResourceManager = UAssetManager::GetInstance();

	DecalMaterial = AssetManager.CreateMaterial(FName("recovery_256x"), FName("Asset/Texture/recovery_256x.png"));

	Type = EPrimitiveType::Decal;
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);

	Indices = ResourceManager.GetIndexData(Type);
	IndexBuffer = ResourceManager.GetIndexbuffer(Type);
	NumIndices = ResourceManager.GetNumIndices(Type);

	BoundingBox = &ResourceManager.GetAABB(Type);

	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::Solid;
}

UDecalComponent::~UDecalComponent()
{
	DecalMaterial = nullptr;
}

void UDecalComponent::SetDecalMaterial(UMaterial* InMaterial)
{
	if (DecalMaterial == InMaterial) { return; }

	DecalMaterial = InMaterial;
}

UMaterial* UDecalComponent::GetDecalMaterial() const
{
	return DecalMaterial;
}

UObject* UDecalComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UDecalComponent*>(Super::Duplicate(Parameters));

	DupObject->DecalMaterial = DecalMaterial;

	return DupObject;
}
