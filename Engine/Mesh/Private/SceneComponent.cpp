#include "pch.h"
#include "Mesh/Public/SceneComponent.h"
#include "Mesh/Public/ResourceManager.h"
#include "Render/Public/Renderer.h"
#include "Global/Constant.h"


void USceneComponent::SetRelativeLocation(const FVector& Location)
{
	RelativeLocation = Location;
}

void USceneComponent::SetRelativeRotation(const FVector& Rotation)
{
	RelativeRotation = Rotation;
}
void USceneComponent::SetRelativeScale3D(const FVector& Scale)
{
	RelativeScale3D = Scale;
}

FVector& USceneComponent::GetRelativeLocation()
{
	return RelativeLocation;
}
FVector& USceneComponent::GetRelativeRotation()
{
	return RelativeRotation;
}
FVector& USceneComponent::GetRelativeScale3D()
{
	return RelativeScale3D;
}

const TArray<FVertex>* UPrimitiveComponent::GetVerticesData() const
{
    UResourceManager& ResourceManager = UResourceManager::GetInstance();
    return ResourceManager.GetVertexData(Type);
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
