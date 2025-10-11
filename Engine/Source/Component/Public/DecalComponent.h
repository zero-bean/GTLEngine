#pragma once
#include "PrimitiveComponent.h"

class UMaterial;

UCLASS()
class UDecalComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

public:
	UDecalComponent();
	virtual ~UDecalComponent();

	void SetDecalMaterial(UMaterial* InMaterial);
	UMaterial* GetDecalMaterial() const;

	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;

private:
	UMaterial* DecalMaterial;
};
