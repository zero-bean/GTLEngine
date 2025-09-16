#include "stdafx.h"
#include "UPrimitiveComponent.h"
#include "UBillboardComponent.h"
#include "UMeshManager.h"
#include "URenderer.h"
#include "ShowFlagManager.h"

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

void UPrimitiveComponent::DrawMesh(URenderer& renderer)
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
}

void UPrimitiveComponent::DrawBillboard(URenderer& renderer)
{
	if (billBoard)
	{
		billBoard->Draw(renderer);
	}
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

void UPrimitiveComponent::Draw(URenderer& renderer, UShowFlagManager* ShowFlagManager)
{
	assert(ShowFlagManager != nullptr);

	if (ShowFlagManager->IsEnabled(EEngineShowFlags::SF_Primitives))
	{
		DrawMesh(renderer);
	}
	
	if (ShowFlagManager->IsEnabled(EEngineShowFlags::SF_BillboardText))
	{
		DrawBillboard(renderer);
	}
}