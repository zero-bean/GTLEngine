#include "stdafx.h"
#include "UBillboardComponent.h"
#include "UPrimitiveComponent.h"
#include "UMeshManager.h"
#include "URenderer.h"
#include "UMesh.h"

IMPLEMENT_UCLASS(UBillboardComponent, USceneComponent)

UBillboardComponent::~UBillboardComponent()
{
    owner = nullptr;
}

bool UBillboardComponent::Init(UMeshManager* meshManager)
{
	if (owner == nullptr)
		return false;

    std::string s;
    int TempUUID = owner->UUID;
    if (TempUUID == 0) 
    { 
        TextDigits = "0"; 
        return true; 
    }

    while (TempUUID > 0) 
    {
        int d = int(TempUUID % 10);
        s.push_back(char('0' + d));
        TempUUID /= 10;
    }
    std::reverse(s.begin(), s.end());
    TextDigits = std::move(s);

    return true;
}

void UBillboardComponent::Draw(URenderer& renderer)
{
    if (!owner || TextDigits.empty()) return;

    // 1) 오브젝트 머리 위 앵커 계산
    FVector anchor = owner->GetWorldLocation();        
    float offsetZ = DigitH; 

    if (UMesh* m = owner->GetMesh())
    {
        if (m->HasPrecomputedAABB())  
        {
            // 로컬 높이(Y) → 월드 스케일 반영
            const FBoundingBox& box = m->GetPrecomputedLocalBox();
            const float localHeightZ = (box.Max.Z - box.Min.Z);  
            offsetZ = 0.5f * localHeightZ * owner->GetScale().Z + DigitH * 0.5f + 0.02f;
        }
    }
    anchor.Z += offsetZ;

    auto R_object = GetQuaternion().ToMatrixRow();
    const float deg = -90.0f;
    float rad = deg * 3.1415926535f / 180.0f;
    FQuaternion rotated = FQuaternion::FromAxisAngle(FVector(1, 0, 0), rad);
    FMatrix converted = rotated.ToMatrixRow();
    // 최종 월드 변환 (S * R * T; row-major 규약)
    const FMatrix M = FMatrix::SRTRowQuaternion(anchor, converted, GetScale());

    // 3) 텍스트 중앙 정렬: 총 폭을 구해 penX = -width/2
    const int n = (int)TextDigits.size();
    const float totalWidth = (n > 0) ? (n * DigitW + (n - 1) * Spacing) : 0.0f;
    float penX = -0.5f * totalWidth;
    const float penY = 0.0f;
    // ★ 카메라 Right 벡터
    const FVector R = renderer.GetBillboardRight();

    // 4) 숫자 글리프들 배치
    for (char ch : TextDigits)
    {
        if (ch < '0' || ch > '9') { penX += (DigitW + Spacing); continue; }
        FConstantFont fontData{};
        fontData.cellSizeX = 1.0f / 16.0f;
        fontData.cellSizeY = 1.0f / 16.0f;
        fontData.atlasCols = 16;
        renderer.UpdateFontConstantBuffer(fontData);

        const FVector baseXY(penX, penY, 0.f);
        const FVector sizeXY(DigitW, DigitH, 0.f);

        const float LocalCenterX = penX + 0.5f * DigitW;
        const FVector GlyphAnchor = anchor + R * LocalCenterX;
        //renderer.SubmitSprite(M, baseXY, sizeXY, FSlicedUV(0, 0, 1, 1), 0, FVector(0, 0, 0), (int)ch);
        //    renderer.SubmitSprite(M, baseXY, sizeXY, uv);
        renderer.SubmitBillboardSprite(GlyphAnchor, DigitW, DigitH, FSlicedUV(0,0,1,1), (int)ch);
        penX += (DigitW + Spacing);
    }
}

UMesh* UBillboardComponent::GetMesh() { return mesh; }

void UBillboardComponent::SetOwner(UPrimitiveComponent* inOwner)
{
    owner = inOwner;
}
