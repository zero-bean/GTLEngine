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
	DupObject->SetShadowsEnabled(bEnableShadows);

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

bool UFireBallComponent::IsOccluded(const UPrimitiveComponent* InReceiver, UPrimitiveComponent*& OutOccluder) const
{
	if (!InReceiver) { return false; }

	OutOccluder = nullptr; // 기본적으로 장애물은 없는 상태로 초기화

	// 0. AABB 박스를 통해 해당 프컴의 중심 좌표를 계산.
	FVector ReceiverMin, ReceiverMax;
	InReceiver->GetWorldAABB(ReceiverMin, ReceiverMax);
	const FVector ReceiverCenterVec3 = (ReceiverMin + ReceiverMax) * 0.5f;
	const FVector4 ReceiverCenter = FVector4(ReceiverCenterVec3.X, ReceiverCenterVec3.Y, ReceiverCenterVec3.Z, 1.0f);

	// 1. 광원의 위치에서 수신자 중심으로 향하는 레이 생성.
	FRay Ray;
	Ray.Origin = FVector4(GetWorldLocation().X, GetWorldLocation().Y, GetWorldLocation().Z, 1.0f); 
	Ray.Direction = ReceiverCenter - Ray.Origin;

	// 2. 거리 계산 및 방향 정규화 (w값의 영향을 받지 않도록 3D 벡터로 수행)
	FVector DirectionVec3(Ray.Direction.X, Ray.Direction.Y, Ray.Direction.Z);
	const float DistanceToReceiver = DirectionVec3.Length();
	DirectionVec3.Normalize();

	Ray.Direction = FVector4(DirectionVec3.X, DirectionVec3.Y, DirectionVec3.Z, 0.0f);
	Ray.Origin += Ray.Direction * 0.001f; // 자기 자신과 충돌 방지를 위한 Epsilon

	// 3. BVH를 사용해 레이캐스트 수행
	UPrimitiveComponent* HitComponent = nullptr;
	float HitDistance = FLT_MAX;
	const bool bHit = UBVHierarchy::GetInstance().Raycast(Ray, HitComponent, HitDistance);

	// 4. 장애물이 있고, 그 장애물이 수신자보다 가깝다면 '가려졌다'고 판단
	if (bHit && HitComponent && HitComponent != InReceiver && HitDistance < DistanceToReceiver)
	{
		// 자기 자신(FireBall)이 장애물로 감지된 경우 무시
		if (HitComponent == this) { return false; }

		OutOccluder = HitComponent; // 장애물 정보 전달
		return true; // 빛이 가려짐!
	}

	return false; // 빛이 가려지지 않음
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
