#include "pch.h"
#include "Mesh/Public/SceneComponent.h"
#include "Mesh/Public/ResourceManager.h"


USceneComponent::USceneComponent()
{
	ComponentType = EComponentType::Scene;
}

void USceneComponent::SetParentAttachment(USceneComponent* NewParent)
{
	if (NewParent == this)
	{
		return;
	}

	if (NewParent == ParentAttachment)
	{
		return;
	}

	//부모의 조상중에 내 자식이 있으면 순환참조 -> 스택오버플로우 일어남.
	for (USceneComponent* Ancester = NewParent; NewParent; Ancester = NewParent->ParentAttachment)
	{
		if (NewParent == this) //조상중에 내 자식이 있다면 조상중에 내가 있을 것임.
			return;
	}

	//부모가 될 자격이 있음, 이제 부모를 바꿈.

	if (ParentAttachment) //부모 있었으면 이제 그 부모의 자식이 아님
	{
		ParentAttachment->RemoveChild(this);
	}

	ParentAttachment = NewParent;

	MarkAsDirty();

}

void USceneComponent::RemoveChild(USceneComponent* ChildDeleted)
{
	Children.erase(std::remove(Children.begin(), Children.end(), this), Children.end());
}

void USceneComponent::MarkAsDirty()
{
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;

	for (USceneComponent* Child : Children)
	{
		Child->MarkAsDirty();
	}
}

UPrimitiveComponent::UPrimitiveComponent()
{
	ComponentType = EComponentType::Primitive;
}

void USceneComponent::SetRelativeLocation(const FVector& Location)
{
	RelativeLocation = Location;
	MarkAsDirty();
}

void USceneComponent::SetRelativeRotation(const FVector& Rotation)
{
	RelativeRotation = Rotation;
	MarkAsDirty();
}
void USceneComponent::SetRelativeScale3D(const FVector& Scale)
{
	RelativeScale3D = Scale;
	MarkAsDirty();
}

const FVector& USceneComponent::GetRelativeLocation() const
{
	return RelativeLocation;
}
const FVector& USceneComponent::GetRelativeRotation() const
{
	return RelativeRotation;
}
const FVector& USceneComponent::GetRelativeScale3D() const
{
	return RelativeScale3D;
}

const FMatrix& USceneComponent::GetWorldTransformMatrix() const
{
	if (bIsTransformDirty)
	{
		WorldTransformMatrix = FMatrix::GetModelMatrix(RelativeLocation, FVector::GetDegreeToRadian(RelativeRotation), RelativeScale3D);


		for (USceneComponent* Ancester = ParentAttachment; Ancester && Ancester->ParentAttachment; Ancester = Ancester->ParentAttachment)
		{
			WorldTransformMatrix *= FMatrix::GetModelMatrix(Ancester->RelativeLocation, FVector::GetDegreeToRadian(Ancester->RelativeRotation), Ancester->RelativeScale3D);
		}

		bIsTransformDirty = false;
	}

	return WorldTransformMatrix;
}

const FMatrix& USceneComponent::GetWorldTransformMatrixInverse() const
{

	if (bIsTransformDirtyInverse)
	{
		WorldTransformMatrixInverse = FMatrix::Identity();
		for (USceneComponent* Ancester = ParentAttachment; Ancester && Ancester->ParentAttachment; Ancester = Ancester->ParentAttachment)
		{
			WorldTransformMatrixInverse = FMatrix::GetModelMatrixInverse(Ancester->RelativeLocation, FVector::GetDegreeToRadian(Ancester->RelativeRotation), Ancester->RelativeScale3D) * WorldTransformMatrixInverse;

		}
		WorldTransformMatrixInverse = WorldTransformMatrixInverse * FMatrix::GetModelMatrixInverse(RelativeLocation, FVector::GetDegreeToRadian(RelativeRotation), RelativeScale3D);

		bIsTransformDirtyInverse = false;
	}

	return WorldTransformMatrixInverse;
}

const TArray<FVertex>* UPrimitiveComponent::GetVerticesData() const
{
    UResourceManager& ResourceManager = UResourceManager::GetInstance();
    return ResourceManager.GetVertexData(Type);
}

ID3D11Buffer* UPrimitiveComponent::GetVertexBuffer() const
{
	return Vertexbuffer;
}

void UPrimitiveComponent::SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
	Topology = InTopology;
}

D3D11_PRIMITIVE_TOPOLOGY UPrimitiveComponent::GetTopology() const
{
	return Topology;
}

//void UPrimitiveComponent::Render(const URenderer& Renderer) const
//{
//	Renderer.RenderPrimitive(Vertexbuffer, NumVertices);
//}


/*
* 리소스는 Manager가 관리하고 component는 참조만 함.
*
*/


USphereComponent::USphereComponent()
{
    UResourceManager& ResourceManager = UResourceManager::GetInstance();
    Type = EPrimitiveType::Sphere;
    Vertices = ResourceManager.GetVertexData(Type);
    Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
    NumVertices = ResourceManager.GetNumVertices(Type);
}

UCubeComponent::UCubeComponent()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = EPrimitiveType::Cube;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
}

ULineComponent::ULineComponent()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = EPrimitiveType::Line;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
}
