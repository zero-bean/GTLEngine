#include "pch.h"
#include "Component/Public/BillboardComponent.h"

#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UBillboardComponent, UPrimitiveComponent)

#include "Actor/Public/Actor.h"


UBillboardComponent::UBillboardComponent()
    : Sprite(nullptr)
    , POwnerActor(GetOwner())
{
    SetName("BillboardComponent");
    Type = EPrimitiveType::Billboard;
}

UBillboardComponent::~UBillboardComponent()
{
    Sprite = nullptr;
    POwnerActor = nullptr;
}

void UBillboardComponent::SetSprite(ELightType LightType)
{
	// jft
	if (BillboardSpriteOptions.size() == 0)
	{
		BillboardSpriteOptions = UAssetManager::GetInstance().GetBillboardSpriteOptions();
	}

	switch (LightType)
	{
	case ELightType::Spotlight:
		SetSprite(BillboardSpriteOptions[1].Texture);
		break;
	}
}

void UBillboardComponent::SetSprite(UTexture* InTexture)
{
    Sprite = InTexture;
}

void UBillboardComponent::UpdateRotationMatrix(const UCamera* InCamera)
{
	const FVector& OwnerActorLocation = GetOwner()->GetActorLocation();
    if (!InCamera)
    {
        return;
    }

    AActor* OwnerActor = POwnerActor ? POwnerActor : GetOwner();
    if (!OwnerActor)
    {
        return;
    }

	FMatrix ViewMatrix = InCamera->GetFViewProjConstantsInverse().View;
	ViewMatrix.Data[3][0] = 0.0f;
	ViewMatrix.Data[3][1] = 0.0f;
	ViewMatrix.Data[3][2] = 0.0f;
	ViewMatrix.Data[3][3] = 1.0f;
	FMatrix WorldTransform = GetWorldTransformMatrix();
	FVector WorldLocation = FVector(WorldTransform.Data[3][0], WorldTransform.Data[3][1], WorldTransform.Data[3][2]);
	RTMatrix = ViewMatrix * FMatrix::TranslationMatrix(WorldLocation);

    // FVector ToCamera = InCamera->GetForward();
    // ToCamera = FVector(-ToCamera.X, -ToCamera.Y, -ToCamera.Z);
    //
    // const FVector4 WorldUp4 = FVector4(0, 0, 1, 1);
    // const FVector WorldUp = { WorldUp4.X, WorldUp4.Y, WorldUp4.Z };
    // FVector Right = WorldUp.Cross(ToCamera);
    // Right.Normalize();
    // FVector Up = ToCamera.Cross(Right);
    // Up.Normalize();
    //
    // RTMatrix = FMatrix(ToCamera, Right, Up);
    //
    // const FVector Translation = OwnerActorLocation + GetRelativeLocation();
    // RTMatrix *= FMatrix::TranslationMatrix(Translation);
}
