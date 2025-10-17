#pragma once
#include"SceneComponent.h"
#include "SceneLoader.h"
class ULightComponent : public USceneComponent
{
public:
	DECLARE_CLASS(ULightComponent, USceneComponent)
	ULightComponent();
	~ULightComponent() override;

	// Serialization for transform
	virtual void Serialize(bool bIsLoading, FComponentData& InOut);

};

