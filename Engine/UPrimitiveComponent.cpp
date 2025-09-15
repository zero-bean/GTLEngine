#include "stdafx.h"
#include "UPrimitiveComponent.h"
#include "UBillboardComponent.h"
#include "UMeshManager.h"
#include "URenderer.h"

IMPLEMENT_UCLASS(UPrimitiveComponent, USceneComponent)
bool UPrimitiveComponent::Init(UMeshManager* meshManager)
{
	if (meshManager)
	{
		mesh = meshManager->RetrieveMesh(GetClass()->GetMeta("MeshName"));
		billBoard = NewObject<UBillboardComponent>();
		billBoard->SetOwner(this);
		billBoard->Init(meshManager);

		return mesh != nullptr;
	}
	return false;
}

void UPrimitiveComponent::UpdateConstantBuffer(URenderer& renderer)
{
	FMatrix M = GetWorldTransform();
	renderer.SetModel(M, Color, bIsSelected);

	if (billBoard)
	{
		billBoard->UpdateConstantBuffer(renderer);
	}
}

UPrimitiveComponent::~UPrimitiveComponent()
{
	SAFE_DELETE(billBoard);
}

void UPrimitiveComponent::Draw(URenderer& renderer)
{
	if (!mesh || !mesh->VertexBuffer)
	{
		return;
	}

	if (mesh->PrimitiveType == D3D11_PRIMITIVE_TOPOLOGY_LINELIST)
	{
		/*UpdateConstantBuffer(renderer);
		renderer.SubmitLineList(mesh);
		return;*/
		// per-object 상수버퍼 업로드 금지!
		const FMatrix M = GetWorldTransform();
		renderer.SubmitLineList(mesh->Vertices, mesh->Indices, M);
		return;
	}
	UpdateConstantBuffer(renderer);
	renderer.DrawMesh(mesh);

	if (billBoard)
	{
		billBoard->Draw(renderer);
	}
}