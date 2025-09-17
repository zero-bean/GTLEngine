#include "stdafx.h"
#include "UPrimitiveComponent.h"
#include "UBillboardComponent.h"
#include "UMeshManager.h"
#include "UMaterialManager.h"
#include "ShowFlagManager.h"
#include "URenderer.h"
#include "UMesh.h"
#include "UMaterial.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UPrimitiveComponent, USceneComponent)
bool UPrimitiveComponent::Init(UMeshManager* meshManager, UMaterialManager* materialManager)
{
	if (meshManager)
	{
		Mesh = meshManager->RetrieveMesh(GetClass()->GetMeta("MeshName"));
		Billboard = NewObject<UBillboardComponent>();
		Billboard->SetOwner(this);
		Billboard->Init(meshManager, materialManager);;
	}

	if (materialManager)
	{
		Material = materialManager->RetrieveMaterial(GetClass()->GetMeta("MaterialName"));
	}

	return Mesh != nullptr || Material != nullptr;
}

void UPrimitiveComponent::DrawMesh(URenderer& renderer)
{
	if (!Mesh || !Mesh->VertexBuffer)
	{
		return;
	}

	if (Mesh->PrimitiveType == D3D11_PRIMITIVE_TOPOLOGY_LINELIST)
	{
		/*UpdateConstantBuffer(renderer);
		renderer.SubmitLineList(mesh);
		return;*/
		// per-object 상수버퍼 업로드 금지!
		const FMatrix M = GetWorldTransform();
		renderer.SubmitLineList(Mesh->Vertices, Mesh->Indices, M);
		return;
	}
	UpdateConstantBuffer(renderer);

	renderer.DrawMesh(Mesh, Material);
}

void UPrimitiveComponent::DrawBillboard(URenderer& renderer)
{
	if (Billboard)
	{
		Billboard->Draw(renderer);
	}
}

void UPrimitiveComponent::UpdateConstantBuffer(URenderer& renderer)
{
	FMatrix M = GetWorldTransform();
	renderer.SetModel(M, Color, bIsSelected);
}

UPrimitiveComponent::~UPrimitiveComponent()
{
	SAFE_DELETE(Billboard);
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