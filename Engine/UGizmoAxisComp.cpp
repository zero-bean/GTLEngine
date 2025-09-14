#include "stdafx.h"
#include "UClass.h"
#include "URenderer.h"
#include "UGizmoAxisComp.h"

IMPLEMENT_UCLASS(UGizmoAxisComp, UGizmoComponent)
UCLASS_META(UGizmoAxisComp, MeshName, "GizmoAxis")

UGizmoAxisComp::UGizmoAxisComp()
{
}

void UGizmoAxisComp::Draw(URenderer& renderer)
{
	if (!mesh || !mesh->VertexBuffer)
	{
		return;
	}

	const FMatrix M = GetWorldTransform();

	UpdateConstantBuffer(renderer);
	renderer.DrawMesh(mesh);
}
