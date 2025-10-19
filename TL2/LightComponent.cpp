#include"pch.h"
#include "LightComponent.h"
#include "SceneLoader.h"

ULightComponent::ULightComponent()
{
}

ULightComponent::~ULightComponent()
{
}

void ULightComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    if (bIsLoading)
    {
        // Load relative transform from component data
        SetRelativeLocation(InOut.RelativeLocation);
        SetRelativeRotation(FQuat::MakeFromEuler(InOut.RelativeRotation));
        SetRelativeScale(InOut.RelativeScale);
    }
    else
    {
        // Save relative transform to component data
        InOut.RelativeLocation = GetRelativeLocation();
        InOut.RelativeRotation = GetRelativeRotation().ToEuler();
        InOut.RelativeScale = GetRelativeScale();
    }
}

void ULightComponent::DrawDebugLines(class URenderer* Renderer)
{
    // Base implementation - derived classes can override
}
