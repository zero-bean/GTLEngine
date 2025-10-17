// DecalSpotLightComponent.h (Inheritance Version)
#pragma once
#include "DecalComponent.h"

class UDecalSpotLightComponent : public UDecalComponent
{
public:
    DECLARE_CLASS(UDecalSpotLightComponent, UDecalComponent)

    UDecalSpotLightComponent();
    const TArray<FVector> GetBoundingBoxLines() const override;
    const FVector4 GetBoundingBoxColor() const override;

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;
    // Spotlight frustum properties, calculated from the component's extents
    float Radius = 0.0f;
    float Height = 0.0f;
    float Near = 0.0f;
    float Far = 0.0f;
    float Fov = 0.0f; // in radians
    float Aspect = 0.0f;

protected:
    void UpdateDecalProjectionMatrix() override;
};