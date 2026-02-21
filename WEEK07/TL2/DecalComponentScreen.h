// DecalComponent.h
// ProjectionDecal과 비교하기 위한 목적으로 남겨둠 사용 시 World.cpp 수정 필요
#pragma once
#include "PrimitiveComponent.h"
#include "StaticMesh.h"

class UDecalComponentScreen : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UDecalComponentScreen, UPrimitiveComponent)

    UDecalComponentScreen();
    virtual ~UDecalComponentScreen() override;

    void Render(URenderer* Renderer, const FMatrix& View, const FMatrix& Proj, FViewport* Viewport) ;

    // 데칼 크기 설정 (박스 볼륨의 크기)
    void SetDecalSize(const FVector& InSize) { DecalSize = InSize; }
    FVector GetDecalSize() const { return DecalSize; }



    // 데칼 텍스처 설정
    void SetDecalTexture( FString TexturePath);

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

    UStaticMesh* GetDecalBoxMesh() const { return DecalBoxMesh; }
protected:
    // 데칼 박스 메쉬 (큐브)
    UStaticMesh* DecalBoxMesh = nullptr;

    // 데칼 크기
    FVector DecalSize = FVector(1.0f, 1.0f, 1.0f);

    // 데칼 블렌드 모드
    enum class EDecalBlendMode
    {
        Translucent,    // 반투명 블렌딩
        Stain,          // 곱셈 블렌딩
        Normal,         // 노멀맵
        Emissive        // 발광
    };

    EDecalBlendMode BlendMode = EDecalBlendMode::Translucent;
};
