#include "Mesh/Public/SceneComponent.h"
#include "Mesh/Public/ResourceManager.h"
#include "Render/Public/Renderer.h"

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

const FVector& USceneComponent::GetRelativeLocation()
{
	return RelativeLocation;
}
const FVector& USceneComponent::GetRelativeRotation()
{
	return RelativeRotation;
}
const FVector& USceneComponent::GetRelativeScale3D()
{
	return RelativeScale3D;
}

const TArray<FVertexSimple>* UPrimitiveComponent::GetVerticesData() const
{
    UResourceManager& ResourceManager = UResourceManager::GetInstance();
    return ResourceManager.GetVertexData(Type);
}

const void UPrimitiveComponent::Render(URenderer& Renderer) const
{
    Renderer.RenderPrimitive(Vertexbuffer, NumVertices);
}


/*
* 리소스는 Manager가 관리하고 component는 참조만 함.
*
*/
UCubeComp::UCubeComp()
{
    UResourceManager& ResourceManager = UResourceManager::GetInstance();
    Type = EPrimitiveType::Cube;
    Vertices = ResourceManager.GetVertexData(Type);
    Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
    NumVertices = ResourceManager.GetNumVertices(Type);
}

USphereComp::USphereComp()
{
    UResourceManager& ResourceManager = UResourceManager::GetInstance();
    Type = EPrimitiveType::Sphere;
    Vertices = ResourceManager.GetVertexData(Type);
    Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
    NumVertices = ResourceManager.GetNumVertices(Type);
}
