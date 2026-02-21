#pragma once
#include "SceneComponent.h"
#include "Vector.h"

class UFXAAComponent : public USceneComponent
{

public:
    DECLARE_CLASS(UFXAAComponent, USceneComponent);

    void Render(URenderer* Renderer);

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

    // Editor Details
    void RenderDetails() override;

    float GetSlideX() {return SlideX ;}
    float GetSpanMax() {return SpanMax;}
    int GetReduceMin() {return ReduceMin;}
    float GetReduceMul() {return ReduceMul;}
    void SetSlideX(float InSlideX) { 
        SlideX = InSlideX; 
    SlideX = SlideX > 1 ? 1 : SlideX;
    SlideX = SlideX < 0 ? 0 : SlideX;
    }
    void SetSpanMax(float InSpanMax) { SpanMax = InSpanMax; }
    void SetReduceMin(int InReduceMin) { 
        ReduceMin = InReduceMin;
        ReduceMin = ReduceMin > 128 ? 128 : ReduceMin;
        ReduceMin = ReduceMin < 0 ? 0 : ReduceMin;
    }
    void SetReduceMul(float InReduceMul) { 
        ReduceMul = InReduceMul; 
        ReduceMul = ReduceMul > 1 ? 1 : ReduceMul;
        ReduceMul = ReduceMul < 0 ? 0 : ReduceMul;
    }

private:
    /*float SlideX = 1.0f;
    float SpanMax = 8.0f;
    float ReduceMin = 1.0f / 128.0f;
    float ReduceMul = 1.0f / 8.0f;*/

    float SlideX = 1.0f; //0~1
    float SpanMax = 8.0f; //
    int ReduceMin = 1; //0~128 => 0~1
    float ReduceMul = 1.0f / 8.0f; //0~1
};

