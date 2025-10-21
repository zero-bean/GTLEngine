#include"pch.h"
#include "LightComponent.h"
#include "SceneLoader.h"

ULightComponent::ULightComponent()
{
    TempColor = MakeFromColorTemperature(Temperature);
    UpdateFinalColor();
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

float ULightComponent::GetColorTemperature() const
{
    return Temperature;
}

void ULightComponent::SetColorTemperature(float InTemperature)
{
    if (Temperature != InTemperature)
    {
        Temperature = InTemperature;
        TempColor = MakeFromColorTemperature(Temperature);        
        UpdateFinalColor();
    }
}

void ULightComponent::SetTintColor(const FLinearColor& InColor)
{
    TintColor = InColor;
    UpdateFinalColor();
}

void ULightComponent::UpdateSpriteColor(const FLinearColor& InSpriteColor)
{
    if (GetOwner() && GetOwner()->SpriteComponent)
    {
        GetOwner()->SpriteComponent->SetSpriteColor(InSpriteColor);
    }
}

void ULightComponent::UpdateFinalColor()
{
    FinalColor = TintColor * TempColor;
}

void ULightComponent::DrawDebugLines(class URenderer* Renderer)
{
    // Base implementation - derived classes can override
}
