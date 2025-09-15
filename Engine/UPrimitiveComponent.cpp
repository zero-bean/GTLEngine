#include "stdafx.h"
#include "UPrimitiveComponent.h"
#include "UMeshManager.h"
#include "URenderer.h"

IMPLEMENT_UCLASS(UPrimitiveComponent, USceneComponent)
bool UPrimitiveComponent::Init(UMeshManager* meshManager)
{
	if (meshManager)
	{
		mesh = meshManager->RetrieveMesh(GetClass()->GetMeta("MeshName"));
		return mesh != nullptr;
	}
	return false;
}

void UPrimitiveComponent::UpdateConstantBuffer(URenderer& renderer)
{
	FMatrix M = GetWorldTransform();
	renderer.SetModel(M, Color, bIsSelected);
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
    // 폰트 분기
    if (bBillboard)
    {
        // 1) Billboard 셰이더/IL
        auto* Ctx = renderer.GetDeviceContext();
        // Font 셰이더/IL 사용
        Ctx->IASetInputLayout(renderer.GetInputLayout("Font"));
        Ctx->VSSetShader(renderer.GetVertexShader("Font"), nullptr, 0);
        Ctx->PSSetShader(renderer.GetPixelShader("Font"), nullptr, 0);

        // 월드 위치(센터) 뽑기
        const FMatrix M = GetWorldTransform();
        const FVector centerWS(M.M[3][0], M.M[3][1], M.M[3][2]);

        // per-object: Center/Size 채우기
        renderer.SetBillboardObject(centerWS, BillboardSizeX, BillboardSizeY);

        // billboard는 단위 쿼드 기준 → M = Identity
        renderer.SetModel(FMatrix::IdentityMatrix(), Color, bIsSelected);

        if (BillboardSRV) renderer.SetTexture(BillboardSRV, 0);

        // 직접 Draw (renderer.DrawMesh는 Default 셰이더를 바인딩하므로 X)
        UINT offset = 0;
        Ctx->IASetVertexBuffers(0, 1, &mesh->VertexBuffer, &mesh->Stride, &offset);
        if (mesh->IndexBuffer && mesh->NumIndices > 0) {
            Ctx->IASetIndexBuffer(mesh->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            Ctx->IASetPrimitiveTopology(mesh->PrimitiveType);
            Ctx->DrawIndexed(mesh->NumIndices, 0, 0);
        }
        else {
            Ctx->IASetPrimitiveTopology(mesh->PrimitiveType);
            Ctx->Draw(mesh->NumVertices, 0);
        }
        return;
    }

	UpdateConstantBuffer(renderer);
	renderer.DrawMesh(mesh);
}