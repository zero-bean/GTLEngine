#include "pch.h"
#include "Component/Public/DecalComponent.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>
#include <filesystem>

#include "Editor/Public/EditorEngine.h"
#include "Actor/Public/Actor.h"

IMPLEMENT_CLASS(UDecalComponent, UPrimitiveComponent)

UDecalComponent::UDecalComponent() : DecalMaterial(nullptr)
{
	UAssetManager& AssetManager = UAssetManager::GetInstance();

	DecalMaterial = AssetManager.CreateMaterial(FName("bullet-hole"), FName("Asset/Texture/bullet-hole.png"));
	SetName("DecalComponent");

	Type = EPrimitiveType::Decal;
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	Vertices = AssetManager.GetVertexData(Type);
	VertexBuffer = AssetManager.GetVertexbuffer(Type);
	NumVertices = AssetManager.GetNumVertices(Type);

	Indices = AssetManager.GetIndexData(Type);
	IndexBuffer = AssetManager.GetIndexbuffer(Type);
	NumIndices = AssetManager.GetNumIndices(Type);

	BoundingBox = &AssetManager.GetAABB(Type);

	// 렌더링 상태를 와이어프레임으로 설정 (DecalActor 생성자에서 가져옴)
	RenderState.FillMode = EFillMode::Solid;
	RenderState.CullMode = ECullMode::None;
}

UDecalComponent::~UDecalComponent()
{
	DecalMaterial = nullptr;
    SafeDelete(DecalMaterial);
    SafeDelete(ProjectionBox);
}

void UDecalComponent::TickComponent(float DeltaSeconds)
{
    UpdateProjectionFromWorldTransform();

    // 페이드 인 & 아웃 업데이트
    if (FadeProperty.Update(DeltaSeconds))
    {
        if (FadeProperty.bDestroyedAfterFade && FadeProperty.bFadeCompleted)
        {
            // Defer deletion to avoid iterator invalidation during ticking
            if (GetOwner()->GetRootComponent() == this)
            {
                // Root component: schedule actor for deletion on the level
                if (GEngine && GEngine->GetCurrentLevel())
                {
                    GEngine->GetCurrentLevel()->MarkActorForDeletion(GetOwner());
                }
            }
            else
            {
                // Non-root component: ask owner to remove after it finishes ticking
                GetOwner()->MarkComponentForRemoval(this);
            }
        }
    }
}

void UDecalComponent::StartFadeIn(float Duration, float Delay, EFadeStyle InFadeStyle)
{
	FadeProperty.FadeInDuration = Duration;
	FadeProperty.FadeInStartDelay = Delay;
	FadeProperty.StartFadeIn(InFadeStyle);
}

void UDecalComponent::StartFadeOut(float Duration, float Delay, bool bDestroyOwner, EFadeStyle InFadeStyle)
{
	FadeProperty.FadeDuration = Duration;
	FadeProperty.FadeStartDelay = Delay;
	FadeProperty.bDestroyedAfterFade = bDestroyOwner;
	FadeProperty.StartFadeOut(InFadeStyle);
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
	DupObject->FadeProperty = FadeProperty;

	return DupObject;
}

void UDecalComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

	// --- 불러오기 ---
	if (bInIsLoading)
	{
		FString MaterialName, TexturePath;
		FJsonSerializer::ReadString(InOutHandle, "DecalMaterialName", MaterialName, "", false);
		FJsonSerializer::ReadString(InOutHandle, "DecalMaterialTexturePath", TexturePath, "", false);

        if (!MaterialName.empty())
        {
            UAssetManager& AssetManager = UAssetManager::GetInstance();
            UMaterial* LoadedMaterial = AssetManager.CreateMaterial(FName(MaterialName), FName(TexturePath));
            SetDecalMaterial(LoadedMaterial);
        }

        UpdateProjectionFromWorldTransform();
    }
	// --- 저장하기 ---
	else
	{
		if (DecalMaterial)
		{
			// 재질의 이름과 텍스처 경로를 가져옵니다.
			const FString& MaterialName = DecalMaterial->GetName().ToString();
			const FString& TexturePath = DecalMaterial->GetDiffuseTexture() ? DecalMaterial->GetDiffuseTexture()->GetFilePath().ToString() : "";
			InOutHandle["DecalMaterialName"] = MaterialName;
			InOutHandle["DecalMaterialTexturePath"] = TexturePath;
		}
	}
}

void UDecalComponent::UpdateProjectionFromWorldTransform()
{
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
