#pragma once
#include "PointLightComponent.h"

struct FComponentData;
struct FSpotLightInfo;


class USpotLightComponent : public UPointLightComponent
{
public:
	DECLARE_CLASS(USpotLightComponent, ULightComponent)

	USpotLightComponent();
	~USpotLightComponent() override;

	FSpotLightInfo SpotData;

protected:
	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

};

