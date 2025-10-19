#include "pch.h"
#include "DirectionalLightComponent.h"
#include "SceneLoader.h"

UDirectionalLightComponent::UDirectionalLightComponent()
{
    bCanEverTick = true;    
}

UDirectionalLightComponent::~UDirectionalLightComponent()
{
}

void UDirectionalLightComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    // Call parent class serialization for transforms
    ULightComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        Intensity = InOut.DirectionalLightProperty.Intensity;
        FinalColor = InOut.DirectionalLightProperty.Color;
        bEnableSpecular = InOut.DirectionalLightProperty.bEnableSpecular;
    }
    else
    {
        InOut.DirectionalLightProperty.Intensity = Intensity;
        InOut.DirectionalLightProperty.Color = FinalColor;
        InOut.DirectionalLightProperty.bEnableSpecular = bEnableSpecular;
    }
}

void UDirectionalLightComponent::TickComponent(float DeltaSeconds)
{
    // Directional light는 보통 고정되어 있지만, 필요시 동적 변화 구현 가능
    // 예: 해의 이동에 따른 방향 변화 등
}

void UDirectionalLightComponent::SetSpecularEnable(bool bEnable)
{
    bEnableSpecular = bEnable ? 1 : 0;
}

UObject* UDirectionalLightComponent::Duplicate()
{
    UDirectionalLightComponent* DuplicatedComponent = NewObject<UDirectionalLightComponent>();
    CopyCommonProperties(DuplicatedComponent);
    DuplicatedComponent->Intensity = this->Intensity;
    DuplicatedComponent->FinalColor = this->FinalColor;    
    DuplicatedComponent->bEnableSpecular = this->bEnableSpecular;
    DuplicatedComponent->DuplicateSubObjects();
    return DuplicatedComponent;
}

void UDirectionalLightComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}
