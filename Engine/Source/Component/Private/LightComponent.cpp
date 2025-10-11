#include "pch.h"
#include "Component/Public/LightComponent.h"

IMPLEMENT_CLASS(ULightComponent, UPrimitiveComponent)
ULightComponent::ULightComponent()
{}

void ULightComponent::UpdateLightColor(FVector4 InColor)
{
	LightColor = InColor;
}
