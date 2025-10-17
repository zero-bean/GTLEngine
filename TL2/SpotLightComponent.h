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

	float GetInnerConeAngle() { return InnerConeAngle;  }
	float GetOuterConeAngle() { return OuterConeAngle;  }

	void SetInnerConeAngle(float Angle) { InnerConeAngle = Angle; }
	void SetOuterConeAngle(float Angle) { OuterConeAngle = Angle; }
protected:
	UObject* Duplicate() override;
	void DuplicateSubObjects() override;
	//FSpotLightInfo SpotData;

	float InnerConeAngle;
	float OuterConeAngle;
};

