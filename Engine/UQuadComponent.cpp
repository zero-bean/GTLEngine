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

bool UQuadComponent::Init(UMeshManager* meshManager)
{
    UUID = 234234324324;
    if (UUID == 0) { TextDigits = "0"; return true; }

    std::string s;
    while (UUID > 0) {
        int d = int(UUID % 10);
        s.push_back(char('0' + d));
        UUID /= 10;
    }
    std::reverse(s.begin(), s.end());
    TextDigits = std::move(s);

    return true;
}

void UQuadComponent::Draw(URenderer& renderer)
{
    if (TextDigits.empty()) return;

    // 이 컴포넌트의 월드 변환
    const FMatrix M = GetWorldTransform();

    // 좌->우로 단순 배치
    float penX = 0.0f;
    const float penY = 0.0f;

    for (char ch : TextDigits)
    {
        if (ch < '0' || ch > '9') { penX += (DigitW + Spacing); continue; }
        const int digit = (int)(ch);

        // 아틀라스에서 해당 숫자의 UV 슬라이스만 얻음
        const FSlicedUV uv = GetFontUV(digit);

        // 월드 좌표계의 좌하단 기준 위치/크기
        const FVector baseXY(penX, 0.0f, 0.f);
        const FVector sizeXY(DigitW, DigitH, 0.f);

        // 스프라이트 배치에 쿼드 제출 (렌더러가 DrawIndexed 한방에 처리)
        renderer.SubmitSprite(
            M,          // 월드 변환(행벡터 규약)
            baseXY,     // 좌하단
            sizeXY,     // (W,H)
            uv     // GetFontUV로 받은 절대 UV
            // pivot 없음: 기본 좌하단 고정
        );

        penX += (DigitW + Spacing);
    }
}
