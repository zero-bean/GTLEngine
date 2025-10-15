#pragma once
#include "PrimitiveComponent.h"

UCLASS()
class UHeightFogComponent :  public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UHeightFogComponent, UPrimitiveComponent)

public:
	UHeightFogComponent();
	virtual ~UHeightFogComponent();

	
	// UObject* Duplicate(FObjectDuplicationParameters Parameters) override;
	// void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
private:
	FHeightFogConstants HeightFogConstants;
};
