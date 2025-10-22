#pragma once
#include "PointLightComponent.h"

struct FComponentData;
struct FSpotLightProperty;


class USpotLightComponent : public UPointLightComponent
{
public:
    DECLARE_SPAWNABLE_CLASS(USpotLightComponent, ULightComponent, "SpotLightComponent")

    USpotLightComponent();
    ~USpotLightComponent() override;

	void Serialize(bool bIsLoading, FComponentData& InOut) override;
	void TickComponent(float DeltaSeconds) override;

	float GetInnerConeAngle() { return InnerConeAngle;  }
	float GetOuterConeAngle() { return OuterConeAngle;  }
	FVector4 GetDirection() { return Direction;  }
    float GetInAndOutSmooth() { return InAnnOutSmooth;  }

	void SetInnerConeAngle(float Angle) { InnerConeAngle = Angle; }
	void SetOuterConeAngle(float Angle) { OuterConeAngle = Angle; }
	void SetDirection(FVector Dir) { Direction = Dir; }
    void SetInAndOutSmooth(float Smooth) { InAnnOutSmooth = Smooth; }

    // Debug/Visualization settings
    void SetCircleSegments(int InSegments) { CircleSegments = InSegments; }
    int  GetCircleSegments() const { return CircleSegments; }

    // Editor Details
    void RenderDetails() override;

    void DrawDebugLines(class URenderer* Renderer, const FMatrix& View, const FMatrix& Proj) override;
    // Debug visualization


protected:
    UObject* Duplicate() override;
    void DuplicateSubObjects() override;
    //FSpotLightInfo SpotData;

    float InnerConeAngle;
    float OuterConeAngle;
    float InAnnOutSmooth;

    FVector4 Direction;

    // Number of points used to draw the end circle for debug visualization
    int CircleSegments = 32;


};

