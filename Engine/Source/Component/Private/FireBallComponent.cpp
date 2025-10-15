#include "pch.h"
#include "Component/Public/FireBallComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Core/Public/BVHierarchy.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>

IMPLEMENT_CLASS(UFireBallComponent, UPrimitiveComponent)

UFireBallComponent::UFireBallComponent()
{
	Type = EPrimitiveType::FireBall;
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	UAssetManager& AssetManager = UAssetManager::GetInstance();

	Vertices = AssetManager.GetVertexData(Type);
	VertexBuffer = AssetManager.GetVertexbuffer(Type);
	NumVertices = AssetManager.GetNumVertices(Type);

	Indices = AssetManager.GetIndexData(Type);
	IndexBuffer = AssetManager.GetIndexbuffer(Type);
	NumIndices = AssetManager.GetNumIndices(Type);

	BoundingBox = &AssetManager.GetAABB(Type);
	SetRadius(Radius);
}

UFireBallComponent::~UFireBallComponent()
{
}

void UFireBallComponent::TickComponent(float DeltaSeconds)
{
	Super::TickComponent(DeltaSeconds);
}

UObject* UFireBallComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UFireBallComponent*>(Super::Duplicate(Parameters));
	DupObject->Color = Color;
	DupObject->Intensity = Intensity;
	DupObject->SetRadius(Radius);
	DupObject->RadiusFallOff = RadiusFallOff;

	return DupObject;
}

void UFireBallComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// --- JSON 파일로부터 데이터를 불러올 때 ---
	if (bInIsLoading)
	{
		FVector4 TempColor;
		FJsonSerializer::ReadVector4(InOutHandle, "Color", TempColor, FVector4::Zero());
		Color.Color = TempColor;

		FJsonSerializer::ReadFloat(InOutHandle, "Intensity", Intensity, 1.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "Radius", Radius, 10.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "RadiusFallOff", RadiusFallOff, 0.1f);
	}
	// --- 현재 데이터를 JSON 파일에 저장할 때 ---
	else
	{
		InOutHandle["Color"] = FJsonSerializer::Vector4ToJson(Color.Color);
		InOutHandle["Intensity"] = Intensity;
		InOutHandle["Radius"] = Radius;
		InOutHandle["RadiusFallOff"] = RadiusFallOff;
	}
}

FLinearColor UFireBallComponent::GetLinearColor() const
{
	return Color;
}

void UFireBallComponent::SetLinearColor(const FLinearColor& InLinearColor)
{
	Color = InLinearColor;

}

void UFireBallComponent::SetRadius(const float& InRadius)
{
	Radius = InRadius;
	SetRelativeScale3D(FVector{ InRadius, InRadius, InRadius });
}
