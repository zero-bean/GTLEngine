#pragma once
#include "PrimitiveComponent.h"
#include "Editor/Public/Camera.h"
#include "Texture/Public/Texture.h"
// jft
#include "Manager/Asset/Public/AssetManager.h"
class AActor;
UCLASS()
class UBillboardComponent : public UPrimitiveComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UBillboardComponent, UPrimitiveComponent)


public:
	UBillboardComponent();
	~UBillboardComponent();

    FMatrix GetRTMatrix() const { return RTMatrix; }

    void UpdateRotationMatrix(const UCamera* InCamera);

    UTexture* GetSprite() const { return Sprite.Get(); }

    void SetSprite(ELightType LightType);
    void SetSprite(UTexture* InTexture);

	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
    FMatrix RTMatrix;
    TObjectPtr<UTexture> Sprite;
    AActor* POwnerActor;
	float ZOffset;

	//jft
	// billboard cache
	TArray<FTextureOption> BillboardSpriteOptions;
};
