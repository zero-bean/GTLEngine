#include "pch.h"
#include "Component/Public/PrimitiveComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"

IMPLEMENT_CLASS(UPrimitiveComponent, USceneComponent)

UPrimitiveComponent::UPrimitiveComponent()
{
	ComponentType = EComponentType::Primitive;
	FVector MinBounds, MaxBounds;
	GetWorldAABB(MinBounds, MaxBounds);
}

UObject* UPrimitiveComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UPrimitiveComponent*>(Super::Duplicate(Parameters));

	// @note 프로퍼티 얕은 복사(Shallow copy)
	DupObject->Vertices		= Vertices;
	DupObject->Indices		= Indices;
	DupObject->VertexBuffer = VertexBuffer;
	DupObject->IndexBuffer	= IndexBuffer;
	DupObject->BoundingBox	= BoundingBox;

	// @note 프로퍼티 깊은 복사(Deep copy)
	DupObject->NumVertices	= NumVertices;
	DupObject->NumIndices	= NumIndices;
	DupObject->Color		= Color;
	DupObject->Topology		= Topology;
	DupObject->RenderState	= RenderState;
	DupObject->Type			= Type;
	DupObject->bVisible		= bVisible;

	return DupObject;
}

const TArray<FNormalVertex>* UPrimitiveComponent::GetVerticesData() const
{
	return Vertices;
}

const TArray<uint32>* UPrimitiveComponent::GetIndicesData() const
{
	return Indices;
}

ID3D11Buffer* UPrimitiveComponent::GetVertexBuffer() const
{
	return VertexBuffer;
}

ID3D11Buffer* UPrimitiveComponent::GetIndexBuffer() const
{
	return IndexBuffer;
}

const uint32 UPrimitiveComponent::GetNumVertices() const
{
	return NumVertices;
}

const uint32 UPrimitiveComponent::GetNumIndices() const
{
	return NumIndices;
}

void UPrimitiveComponent::SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
	Topology = InTopology;
}

D3D11_PRIMITIVE_TOPOLOGY UPrimitiveComponent::GetTopology() const
{
	return Topology;
}

void UPrimitiveComponent::GetWorldAABB(FVector& OutMin, FVector& OutMax) const
{
	if (!BoundingBox)	return;

	if (BoundingBox->GetType() == EBoundingVolumeType::AABB)
	{
		const FAABB* LocalAABB = static_cast<const FAABB*>(BoundingBox);
		FVector LocalCorners[8] =
		{
			FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Min.Z),
			FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Min.Z),
			FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Max.Z),
			FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Max.Z)
		};
		const FMatrix& WorldTransform = GetWorldTransformMatrix();
		FVector WorldMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
		FVector WorldMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (int i = 0; i < 8; i++)
		{
			FVector4 WorldCorner = FVector4(LocalCorners[i].X, LocalCorners[i].Y, LocalCorners[i].Z, 1.0f) * WorldTransform;
			WorldMin.X = std::min(WorldMin.X, WorldCorner.X);
			WorldMin.Y = std::min(WorldMin.Y, WorldCorner.Y);
			WorldMin.Z = std::min(WorldMin.Z, WorldCorner.Z);
			WorldMax.X = std::max(WorldMax.X, WorldCorner.X);
			WorldMax.Y = std::max(WorldMax.Y, WorldCorner.Y);
			WorldMax.Z = std::max(WorldMax.Z, WorldCorner.Z);
		}
		OutMin = WorldMin;
		OutMax = WorldMax;
	}
}

/*
* 리소스는 Manager가 관리하고 component는 참조만 함.
*
*/
