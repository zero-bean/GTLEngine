#pragma once
#include "Info.h"
class UFXAAComponent;

class AFXAAActor : public AInfo
{
public:

	DECLARE_SPAWNABLE_CLASS(AFXAAActor, AInfo, "FXAA");
	AFXAAActor();
	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

private:
	UFXAAComponent* FXAAComponent = nullptr;
	UBillboardComponent* SpriteComponent = nullptr;

};

