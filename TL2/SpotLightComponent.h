#pragma once
#include "PointLightComponent.h"

struct FComponentData;
struct FSpotLightInfo;


class USpotLightComponent : public UPointLightComponent
{
public:
    DECLARE_CLASS(USpotLightComponent, ULightComponent)

    USpotLightComponent();
    ~USpotLightComponent() override;

	float GetInnerConeAngle() { return InnerConeAngle;  }
	float GetOuterConeAngle() { return OuterConeAngle;  }
	FVector4 GetDirection() { return Direction;  }
    float GetInAndOutSmooth() { return InAntOutSmooth;  }
    FVector GetAttFactor() { return AttFactor;  }

	void SetInnerConeAngle(float Angle) { InnerConeAngle = Angle; }
	void SetOuterConeAngle(float Angle) { OuterConeAngle = Angle; }
	void SetDirection(FVector Dir) { Direction = Dir; }
    void SetInAndOutSmooth(float Smooth) { InAntOutSmooth = Smooth; }
    void SetAttFactor(FVector Att) { AttFactor = Att; }

    // Debug/Visualization settings
    void SetCircleSegments(int InSegments) { CircleSegments = InSegments; }
    int  GetCircleSegments() const { return CircleSegments; }

    // Editor Details
    void RenderDetails() override;

    void DrawDebugLines(class URenderer* Renderer) override;
    // Debug visualization


protected:
    UObject* Duplicate() override;
    void DuplicateSubObjects() override;
    //FSpotLightInfo SpotData;

    float InnerConeAngle;
    float OuterConeAngle;
    float InAntOutSmooth;

    FVector4 Direction;
    FVector AttFactor;


    // Number of points used to draw the end circle for debug visualization
    int CircleSegments = 32;


};

