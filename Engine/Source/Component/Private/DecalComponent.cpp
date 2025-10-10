#include "pch.h"
#include "Component/Public/DecalComponent.h"
#include "Texture/Public/Material.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UDecalComponent, UPrimitiveComponent)

UDecalComponent::UDecalComponent() : DecalMaterial(nullptr)
{
	if (DecalMaterial = NewObject<UMaterial>())
	{
		UAssetManager& AssetManager = UAssetManager::GetInstance();
		UTexture* DiffuseTexture = AssetManager.CreateTexture(FName("Asset/Texture/recovery_256x.png"), FName("recovery_256x"));
		DecalMaterial->SetDiffuseTexture(DiffuseTexture);
	}

	UAssetManager& ResourceManager = UAssetManager::GetInstance();

	Type = EPrimitiveType::Decal;
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);

	Indices = ResourceManager.GetIndexData(Type);
	IndexBuffer = ResourceManager.GetIndexbuffer(Type);
	NumIndices = ResourceManager.GetNumIndices(Type);

	const FAABB* AABB = &ResourceManager.GetAABB(Type);
	BoundingBox = AABB;

	FVector minV = AABB->Min;
	FVector maxV = AABB->Max;

	if (minV.X > maxV.X) std::swap(minV.X, maxV.X);
	if (minV.Y > maxV.Y) std::swap(minV.Y, maxV.Y);
	if (minV.Z > maxV.Z) std::swap(minV.Z, maxV.Z);

	FOBB* OBB = new FOBB;
	OBB->Center      = (minV + maxV) * 0.5f;
	OBB->Extents     = (maxV - minV) * 0.5f;   // half-size on each axis
	OBB->Orientation = FMatrix::Identity();      // axis-aligned → identity
	ProjectionBox = OBB;

	// 렌더링 상태를 와이어프레임으로 설정 (DecalActor 생성자에서 가져옴)
	RenderState.FillMode = EFillMode::Solid;
	RenderState.CullMode = ECullMode::None;
}

UDecalComponent::~UDecalComponent()
{
	SafeDelete(DecalMaterial);
	ProjectionBox = nullptr;
}

void UDecalComponent::TickComponent(float DeltaSeconds)
{
	ProjectionBox->Center = GetOwner()->GetActorLocation() + GetRelativeLocation();
	ProjectionBox->Extents = GetRelativeScale3D() * 0.5f;
	FMatrix RotationMatrix = FMatrix::RotationMatrix(GetRelativeRotation());
	ProjectionBox->Orientation = RotationMatrix;
}

void UDecalComponent::SetDecalMaterial(UMaterial* InMaterial)
{
	DecalMaterial = InMaterial;
}

UMaterial* UDecalComponent::GetDecalMaterial() const
{
	return DecalMaterial;
}
