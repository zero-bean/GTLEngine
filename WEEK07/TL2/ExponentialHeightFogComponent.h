#pragma once
#include "SceneComponent.h"

class UExponentialHeightFogComponent : public USceneComponent
{

public:
    DECLARE_CLASS(UExponentialHeightFogComponent, USceneComponent);

    struct FFogInfo
    {
        float FogDensity;
        float FogHeightFalloff;
        float StartDistance;
        float FogCutoffDistance;
        float FogMaxOpacityDistance;
        float FogMaxOpacity;

        FLinearColor FogInscatteringColor;
    };

    void Render(URenderer* Renderer, const FVector& CameraPosition, const FMatrix& View, const FMatrix& Projection, FViewport* Viewport);
    FFogInfo GetFogInfo() const {
        return FFogInfo(
            FogDensity,
            FogHeightFalloff,
            StartDistance,
            FogCutoffDistance,
            FogMaxOpacityDistance,
            FogMaxOpacity,
            FogInscatteringColor);
    }
    void SetFogInfo(const FFogInfo& Info){
        FogDensity = Info.FogDensity;
        FogHeightFalloff = Info.FogHeightFalloff;
        StartDistance = Info.StartDistance;
        FogCutoffDistance = Info.FogCutoffDistance;
        FogMaxOpacityDistance = Info.FogMaxOpacityDistance;
        FogMaxOpacity = Info.FogMaxOpacity;
        FogInscatteringColor = Info.FogInscatteringColor;
    }
    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

    // Editor Details
    void RenderDetails() override;

private:
    float FogDensity = 1.0f;
    float FogHeightFalloff = 0.003f;
    float StartDistance = 0.0f;
    float FogCutoffDistance = 10000.0f;
    float FogMaxOpacityDistance = 10000.0f;
    float FogMaxOpacity = 1.0f;

    FLinearColor FogInscatteringColor{ 0.53f,0.8f,0.92f,1.0 };
};

    