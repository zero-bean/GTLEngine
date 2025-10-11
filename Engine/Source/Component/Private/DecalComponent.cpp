#include "pch.h"
#include "Component/Public/DecalComponent.h"
#include "Texture/Public/Material.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UDecalComponent, UPrimitiveComponent)

UDecalComponent::UDecalComponent() : DecalMaterial(nullptr)
{
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	UAssetManager& ResourceManager = UAssetManager::GetInstance();

	DecalMaterial = AssetManager.CreateMaterial(FName("recovery_256x"), FName("Asset/Texture/recovery_256x.png"));

	Type = EPrimitiveType::Decal;
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);

	Indices = ResourceManager.GetIndexData(Type);
	IndexBuffer = ResourceManager.GetIndexbuffer(Type);
	NumIndices = ResourceManager.GetNumIndices(Type);

	BoundingBox = &ResourceManager.GetAABB(Type);

	// 렌더링 상태를 와이어프레임으로 설정 (DecalActor 생성자에서 가져옴)
	RenderState.FillMode = EFillMode::Solid;
	RenderState.CullMode = ECullMode::None;
    // Initialize projection OBB from current world transform
    {
        const FMatrix& M = GetWorldTransformMatrix();

        FVector axisX(M.Data[0][0], M.Data[0][1], M.Data[0][2]);
        FVector axisY(M.Data[1][0], M.Data[1][1], M.Data[1][2]);
        FVector axisZ(M.Data[2][0], M.Data[2][1], M.Data[2][2]);

        float sx = std::sqrt(axisX.X * axisX.X + axisX.Y * axisX.Y + axisX.Z * axisX.Z);
        float sy = std::sqrt(axisY.X * axisY.X + axisY.Y * axisY.Y + axisY.Z * axisY.Z);
        float sz = std::sqrt(axisZ.X * axisZ.X + axisZ.Y * axisZ.Y + axisZ.Z * axisZ.Z);

        // Avoid divide-by-zero; fall back to identity axis if degenerate
        FVector oX = (sx > 1e-6f) ? (axisX / sx) : FVector(1.f, 0.f, 0.f);
        FVector oY = (sy > 1e-6f) ? (axisY / sy) : FVector(0.f, 1.f, 0.f);
        FVector oZ = (sz > 1e-6f) ? (axisZ / sz) : FVector(0.f, 0.f, 1.f);

        FVector extents(0.5f * sx, 0.5f * sy, 0.5f * sz);
        FVector center(M.Data[3][0], M.Data[3][1], M.Data[3][2]);

        if (!ProjectionBox)
        {
            ProjectionBox = new FOBB();
        }
        ProjectionBox->Center = center;
        ProjectionBox->Extents = extents;
        ProjectionBox->Orientation = FMatrix(oX, oY, oZ);
    }
}

UDecalComponent::~UDecalComponent()
{
	DecalMaterial = nullptr;
    SafeDelete(DecalMaterial);
    SafeDelete(ProjectionBox);
}

void UDecalComponent::TickComponent(float DeltaSeconds)
{
    // Keep ProjectionBox in sync with the component's world transform
    const FMatrix& M = GetWorldTransformMatrix();

    FVector axisX(M.Data[0][0], M.Data[0][1], M.Data[0][2]);
    FVector axisY(M.Data[1][0], M.Data[1][1], M.Data[1][2]);
    FVector axisZ(M.Data[2][0], M.Data[2][1], M.Data[2][2]);

    float sx = std::sqrt(axisX.X * axisX.X + axisX.Y * axisX.Y + axisX.Z * axisX.Z);
    float sy = std::sqrt(axisY.X * axisY.X + axisY.Y * axisY.Y + axisY.Z * axisY.Z);
    float sz = std::sqrt(axisZ.X * axisZ.X + axisZ.Y * axisZ.Y + axisZ.Z * axisZ.Z);

    FVector oX = (sx > 1e-6f) ? (axisX / sx) : FVector(1.f, 0.f, 0.f);
    FVector oY = (sy > 1e-6f) ? (axisY / sy) : FVector(0.f, 1.f, 0.f);
    FVector oZ = (sz > 1e-6f) ? (axisZ / sz) : FVector(0.f, 0.f, 1.f);

    FVector extents(0.5f * sx, 0.5f * sy, 0.5f * sz);
    FVector center(M.Data[3][0], M.Data[3][1], M.Data[3][2]);

    if (!ProjectionBox)
    {
        ProjectionBox = new FOBB();
    }
    ProjectionBox->Center = center;
    ProjectionBox->Extents = extents;
    ProjectionBox->Orientation = FMatrix(oX, oY, oZ);
}

void UDecalComponent::SetDecalMaterial(UMaterial* InMaterial)
{
	if (DecalMaterial == InMaterial) { return; }

	DecalMaterial = InMaterial;
}

UMaterial* UDecalComponent::GetDecalMaterial() const
{
	return DecalMaterial;
}

UObject* UDecalComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UDecalComponent*>(Super::Duplicate(Parameters));

	DupObject->DecalMaterial = DecalMaterial;

	return DupObject;
}
