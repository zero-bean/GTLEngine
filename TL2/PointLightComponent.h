#pragma once
#include"PrimitiveComponent.h"
#include"LightComponent.h"
#include"LinearColor.h"

struct FComponentData;
struct FPointLightProperty;

class UPointLightComponent : public ULightComponent
{
public:
	DECLARE_CLASS(UPointLightComponent, ULightComponent)
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

    FPointLightProperty FireData;
 

    // 🔸 CPU → GPU 전달용 라이트 데이터 캐시
   // FPointLightData PointLightBuffer;

    
protected:
	

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;
};
