#include "stdafx.h"
#include "UClass.h"
#include "UQuadComponent.h"
#include "UPrimitiveComponent.h"

IMPLEMENT_UCLASS(UQuadComponent, UPrimitiveComponent)
UCLASS_META(UQuadComponent, DisplayName, "Quad")
UCLASS_META(UQuadComponent, MeshName, "Quad")


bool UQuadComponent::Init(UMeshManager* meshManager)
{
    // B) Init에서 보장하고 싶으면 이렇게 (선택)
    bool ok = Super::Init(meshManager);
    // 생성자에서 이미 켰지만, 안전하게 한 번 더 세팅해도 무해함
    SetBillboardEnabled(true);
    // 필요 시 메타/설정값으로 사이즈를 다르게 주고 싶다면 여기서 변경
    // SetBillboardSize(Width, Height);
    // 텍스처 매니저가 있으면 여기서 SRV를 받아서 SetBillboardTexture(...) 해도 좋음
    return ok;
}