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
    // 🔹 Render 함수 (Renderer에서 호출)
	//virtual void Render(URenderer* Renderer, const FMatrix& View, const FMatrix& Proj, const EEngineShowFlags ShowFlags) override;

    // 🔹 Update 함수 (필요시 시간 기반 변화)
    virtual void TickComponent(float DeltaSeconds) override;

    // 🔹 AABB 반환 (충돌/선택 처리용)
    //virtual const FAABB GetWorldAABB() const override;  
  
    // 🔸 CPU → GPU 전달용 라이트 데이터 캐시
    //FPointLightData PointLightBuffer;

    float GetRadius() { return Radius; }
    void SetRadius(float R) { Radius = R; }
    
    float GetRadiusFallOff() { return RadiusFallOff; }
    void SetRadiusFallOff(float FallOff) { RadiusFallOff = FallOff; } 

protected:
    UObject* Duplicate() override;
    void DuplicateSubObjects() override;
    
protected: 
    float Radius = 15.0f;             // 영향 반경
    float RadiusFallOff = 2.0f;       // 감쇠 정도 (클수록 급격히 사라짐) 
 
};
