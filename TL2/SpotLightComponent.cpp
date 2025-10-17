#include "pch.h"
#include "SpotLightComponent.h"

USpotLightComponent::USpotLightComponent() : Direction(1.0, 0.0f, 0.0f, 0.0f), InnerConeAngle(60.0), OuterConeAngle(80.0), InAntOutSmooth(1)
{ 

}

USpotLightComponent::~USpotLightComponent()
{
}

UObject* USpotLightComponent::Duplicate()
{
	return nullptr;
}

void USpotLightComponent::DuplicateSubObjects()
{
}
