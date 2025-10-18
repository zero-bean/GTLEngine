#include "pch.h"
#include "SpotLightComponent.h"

USpotLightComponent::USpotLightComponent() : Direction(1.0, 0.0f, 0.0f, 0.0f), InnerConeAngle(10.0), OuterConeAngle(30.0), AttFactor( 0, 0, 1), InAntOutSmooth(1)
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
