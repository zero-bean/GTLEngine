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
        UE_LOG("Temp %f %f %f", TempColor.R, TempColor.G, TempColor.B);
        UE_LOG("Tint %f %f %f", TintColor.R, TintColor.G, TintColor.B);
        UE_LOG("Final %f %f %f", FinalColor.R, FinalColor.G, FinalColor.B);
    }
}

void ULightComponent::SetTintColor(const FLinearColor& InColor)
{
    TintColor = InColor;
    UpdateFinalColor();
}

void ULightComponent::UpdateFinalColor()
{
    FinalColor = TintColor * TempColor;
}



void ULightComponent::DrawDebugLines(class URenderer* Renderer)
{
    // Base implementation - derived classes can override
}
