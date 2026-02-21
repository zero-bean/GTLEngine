#include"pch.h"
#include "LightComponent.h"
#include "SceneLoader.h"

// Static global default
bool ULightComponent::GShowLightDebugLines = false;

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
        SetIntensity(InOut.LightProperty.Intensity);
        SetColorTemperature(InOut.LightProperty.Temperature);
        this->FinalColor = InOut.LightProperty.FinalColor;
        SetTintColor(InOut.LightProperty.TintColor);
        this->TempColor = InOut.LightProperty.TempColor;
        SetDebugLineEnable(InOut.LightProperty.bEnableDebugLine);
    }
    else
    {
        // Save relative transform to component data
        InOut.RelativeLocation = GetRelativeLocation();
        InOut.RelativeRotation = GetRelativeRotation().ToEuler();
        InOut.RelativeScale = GetRelativeScale();
        InOut.LightProperty.Intensity = GetIntensity();
        InOut.LightProperty.Temperature = GetColorTemperature();
        InOut.LightProperty.FinalColor = GetFinalColor();
        InOut.LightProperty.TintColor = GetTintColor();
        InOut.LightProperty.TempColor = TempColor;
        InOut.LightProperty.bEnableDebugLine = bEnableDebugLine;
    }
    
}

UObject* ULightComponent::Duplicate()
{
    ULightComponent* DuplicatedComponent = NewObject<ULightComponent>();

    CopyCommonProperties(DuplicatedComponent);

    CopyLightProperties(DuplicatedComponent);    
    
    DuplicatedComponent->DuplicateSubObjects();
    return DuplicatedComponent;
}

void ULightComponent::DuplicateSubObjects()
{
    USceneComponent::DuplicateSubObjects();
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

void ULightComponent::CopyLightProperties(ULightComponent* Source)
{
    Source->SetIntensity(Intensity);
    // tintcolor랑 temperature만 있으면 finalcolor 계산 가능
    Source->SetColorTemperature(Temperature);    
    Source->SetTintColor(TintColor);    
    Source->SetDebugLineEnable(false);
}

void ULightComponent::UpdateFinalColor()
{
    FinalColor = TintColor * TempColor;
}

void ULightComponent::DrawDebugLines(class URenderer* Renderer, const FMatrix& View, const FMatrix& Proj)
{
    // Base implementation - derived classes can override
}
