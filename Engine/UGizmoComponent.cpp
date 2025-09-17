#include "stdafx.h"
#include "UGizmoComponent.h"
#include "UMeshManager.h"
#include "URenderer.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UGizmoComponent, USceneComponent)

bool UGizmoComponent::Init(UMeshManager* meshManager)
{
	if (meshManager)
	{
		Mesh = meshManager->RetrieveMesh(GetClass()->GetMeta("MeshName"));
		return Mesh != nullptr;
	}
	return false;
}

FMatrix UGizmoComponent::GetWorldTransform()
{
	return FMatrix::SRTRowQuaternion(RelativeLocation, (OriginQuaternion * RelativeQuaternion).ToMatrixRow(), RelativeScale3D);
}
 
void UGizmoComponent::UpdateConstantBuffer(URenderer& renderer)
{
	FMatrix M = GetWorldTransform();
	renderer.SetModel(M, GetColor(), bIsSelected);
}

void UGizmoComponent::Update(float deltaTime)
{
}

void UGizmoComponent::Draw(URenderer& renderer)
{
	if (!Mesh || !Mesh->VertexBuffer)
	{
		return;
	}

	if (Mesh->PrimitiveType == D3D10_PRIMITIVE_TOPOLOGY_LINELIST)
	{
		const FMatrix M = GetWorldTransform();
		renderer.SubmitLineList(Mesh->Vertices, Mesh->Indices, M);
		return;
	}

	UpdateConstantBuffer(renderer);
	renderer.DrawMesh(Mesh, nullptr);
}

void UGizmoComponent::DrawOnTop(URenderer& renderer)
{
	if (!Mesh || !Mesh->VertexBuffer)
	{
		return;
	}

	UpdateConstantBuffer(renderer);
	renderer.DrawMeshOnTop(Mesh);
}