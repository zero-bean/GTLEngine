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

	virtual void SetParentAttachment(USceneComponent* SceneComponent) override;

    FMatrix GetRTMatrix() const { return RTMatrix; }

    void UpdateRotationMatrix(const UCamera* InCamera);

    UTexture* GetSprite() const { return Sprite.Get(); }

    void SetSprite(ELightType LightType);
    void SetSprite(UTexture* InTexture);
	void SetAboveActor();
private:
    FMatrix RTMatrix;
    TObjectPtr<UTexture> Sprite;
    AActor* POwnerActor;
	float ZOffset;

	//jft
	// billboard cache
	TArray<FTextureOption> BillboardSpriteOptions;
};
