#pragma once
#include"PrimitiveComponent.h"
#include"LightComponent.h"
#include"LinearColor.h"

struct FComponentData;
struct FPointLightProperty;

class UPointLightComponent : public ULightComponent
{
public:
	DECLARE_SPAWNABLE_CLASS(UPointLightComponent, ULightComponent, "Point Light Component")
	UPointLightComponent();
    ~UPointLightComponent() override;
    // 🔹 PointLight의 물리적/시각적 속성
   
    virtual void Serialize(bool bIsLoading, FComponentData& InOut) override;
    // 🔹 Update 함수 (필요시 시간 기반 변화)
    virtual void TickComponent(float DeltaSeconds) override;

    float GetRadius() { return Radius; }
    void SetRadius(float R) { Radius = R; }
    
    float GetRadiusFallOff() { return RadiusFallOff; }
    void SetRadiusFallOff(float FallOff) { RadiusFallOff = FallOff; }

    // Editor Details
    void RenderDetails() override;

    void RenderAttenuationRadius(URenderer* Renderer);

    void DrawDebugLines(class URenderer* Renderer) override;

protected:
    UObject* Duplicate() override;
    void DuplicateSubObjects() override;
    
protected: 
    float Radius = 3.0f;             // 영향 반경
    float RadiusFallOff = 2.0f;       // 감쇠 정도 (클수록 급격히 사라짐) 
 
};
