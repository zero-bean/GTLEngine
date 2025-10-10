#pragma once
#include "PrimitiveComponent.h"
#include "Physics/Public/OBB.h"

class UMaterial;

UCLASS()
class UDecalComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

public:
	UDecalComponent();
	virtual ~UDecalComponent();
	void TickComponent(float DeltaSeconds) override;

	void SetDecalMaterial(UMaterial* InMaterial);
	UMaterial* GetDecalMaterial() const;
	FOBB* GetProjectionBox() const { return ProjectionBox; }

private:
	UMaterial* DecalMaterial;
	FOBB* ProjectionBox;
};
