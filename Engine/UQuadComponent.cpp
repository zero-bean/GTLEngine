#include "stdafx.h"
#include "UClass.h"
#include "UQuadComponent.h"
#include "UPrimitiveComponent.h"

IMPLEMENT_UCLASS(UQuadComponent, UPrimitiveComponent)
UCLASS_META(UQuadComponent, DisplayName, "Quad")
UCLASS_META(UQuadComponent, MeshName, "Quad")

void UQuadComponent::UpdateConstantBuffer(URenderer& renderer)
{
    // 1) 모델행렬 등 기본 상수 업로드
    UPrimitiveComponent::UpdateConstantBuffer(renderer);

    // 2) 글리프 UVRect 계산 → 렌더러에 전달(b1)
    // 48 ~ 57 == 숫자
    FSlicedUV s = GetFontUV(48);
    renderer.UpdateFontConstantBuffer({ s.u0, s.v0, s.u1, s.v1 });
}
