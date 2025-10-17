#pragma once
#include "Info.h"
class UExponentialHeightFogComponent;

class AExponentialHeightFog : public AInfo
{
public:

	DECLARE_SPAWNABLE_CLASS(AExponentialHeightFog, AInfo, "Height Fog");


	AExponentialHeightFog();
	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

private:
	UExponentialHeightFogComponent* HeightFogComponent = nullptr;
	UBillboardComponent* SpriteComponent = nullptr;

};

