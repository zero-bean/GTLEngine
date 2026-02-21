#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class USkeletalMeshComponent;
class UMaterial;

UCLASS()
class USkeletalMeshComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(USkeletalMeshComponentWidget, UWidget)

public:
	void Initialize() override {}
	void Update() override {}
	void RenderWidget() override;

private:
	USkeletalMeshComponent* SkeletalMeshComponent{};

	// Helper functions for rendering different sections
	void RenderSkeletalMeshSelector();
	void RenderMaterialSections();
	void RenderAvailableMaterials(int32 TargetSlotIndex);
	void RenderOptions();

	// Material utility functions
	FString GetMaterialDisplayName(UMaterial* Material) const;
	UTexture* GetPreviewTextureForMaterial(UMaterial* Material) const;
};