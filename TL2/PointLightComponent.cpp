#include"pch.h"
#include "PointLightComponent.h"
#include "Renderer.h"
#include "World.h"
#include"SceneLoader.h"


UPointLightComponent::UPointLightComponent()
{
    // 초기 라이트 데이터 세팅
    FVector WorldPos = GetWorldLocation();
    //PointLightBuffer.Position = FVector4(WorldPos, PointData.Radius);
    //PointLightBuffer.Color = FVector4(PointData.Color.R, PointData.Color.G, PointData.Color.B, PointData.Intensity);
    //PointLightBuffer.FallOff = PointData.RadiusFallOff;

    bCanEverTick = true;
}

UPointLightComponent::~UPointLightComponent()
{
}

void UPointLightComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    // Call parent class serialization for transforms
    ULightComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        Radius = InOut.PointLightProperty.Radius;
        RadiusFallOff = InOut.PointLightProperty.RadiusFallOff;
    }
    else
    {
        InOut.PointLightProperty.Radius = Radius;
        InOut.PointLightProperty.RadiusFallOff = RadiusFallOff;
    }
}


void UPointLightComponent::TickComponent(float DeltaSeconds)
{
    static int Time;
    Time += DeltaSeconds;
   // Radius = 300.0f + sinf(Time) * 100.0f;

    //// 🔹 GPU 업로드용 버퍼 갱신
    //FVector WorldPos = GetWorldLocation();
    //PointLightBuffer.Position = FVector4(WorldPos, PointData.Radius);
    //PointLightBuffer.Color = FVector4(PointData.Color.R, PointData.Color.G, PointData.Color.B, PointData.Intensity);
    //PointLightBuffer.FallOff = PointData.RadiusFallOff;
}

//const FAABB UPointLightComponent::GetWorldAABB() const
//{
//    // PointLight의 Radius를 기반으로 AABB 생성
//    FVector WorldLocation = GetWorldLocation();
//    FVector Extent(PointData.Radius, PointData.Radius, PointData.Radius);
//
//    return FAABB(WorldLocation - Extent, WorldLocation + Extent);
//}

UObject* UPointLightComponent::Duplicate()
{
    UPointLightComponent* DuplicatedComponent = NewObject<UPointLightComponent>();
    CopyCommonProperties(DuplicatedComponent);
     DuplicatedComponent->Radius =this->Radius; // 복제 (단, UObject 포인터 복사는 주의)
    DuplicatedComponent->RadiusFallOff = this->RadiusFallOff; // 복제 (단, UObject 포인터 복사는 주의)
    DuplicatedComponent->DuplicateSubObjects();
    return DuplicatedComponent;
}

void UPointLightComponent::DuplicateSubObjects()
{
	Super_t::DuplicateSubObjects();
}


