#include "pch.h"
#include "Component/Public/SpotLightComponent.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(USpotLightComponent, ULightComponent)

USpotLightComponent::USpotLightComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();

	Type = EPrimitiveType::Spotlight;
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

UObject* USpotLightComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<USpotLightComponent*>(Super::Duplicate(Parameters));

	return DupObject;
}

void USpotLightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);
}
