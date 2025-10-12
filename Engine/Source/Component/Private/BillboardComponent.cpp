#include "pch.h"
#include "Component/Public/BillboardComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Texture/Public/TextureRenderProxy.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>

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

UObject* UBillboardComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UBillboardComponent*>(Super::Duplicate(Parameters));

	DupObject->Sprite = Sprite;
	DupObject->ZOffset = ZOffset;
	DupObject->RTMatrix = RTMatrix;
	DupObject->POwnerActor = POwnerActor;
	DupObject->BillboardSpriteOptions = BillboardSpriteOptions;

	return DupObject;
}

void UBillboardComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	// 부모 클래스의 Serialize 함수를 먼저 호출합니다.
	Super::Serialize(bInIsLoading, InOutHandle);

	// --- JSON 파일로부터 데이터를 불러올 때 ---
	if (bInIsLoading)
	{
		FString TexturePath;
		// "SpriteTexturePath" 키로 저장된 텍스처 경로를 읽어옵니다.
		FJsonSerializer::ReadString(InOutHandle, "SpriteTexturePath", TexturePath, "", false);

		// 텍스처 경로가 비어있지 않다면 에셋 매니저를 통해 텍스처를 로드합니다.
		if (!TexturePath.empty())
		{
			UAssetManager& AssetManager = UAssetManager::GetInstance();
			if (UTexture* LoadedTexture = AssetManager.CreateTexture(FName(TexturePath)))
			{
				SetSprite(LoadedTexture);
			}
		}

		// "ZOffset" 키로 저장된 float 값을 읽어와 멤버 변수에 할당합니다.
		FJsonSerializer::ReadFloat(InOutHandle, "ZOffset", ZOffset);
	}
	// --- 현재 데이터를 JSON 파일에 저장할 때 ---
	else
	{
		// Sprite 텍스처가 유효한지 확인합니다.
		if (Sprite)
		{
			// 텍스처의 파일 경로를 FString으로 가져옵니다.
			const FString& TexturePath = Sprite->GetFilePath().ToString();
			// "SpriteTexturePath" 라는 키로 텍스처 경로를 JSON에 저장합니다.
			InOutHandle["SpriteTexturePath"] = TexturePath;
		}

		// ZOffset 값을 "ZOffset" 키로 JSON에 저장합니다.
		InOutHandle["ZOffset"] = ZOffset;
	}
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
