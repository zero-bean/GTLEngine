#pragma once
#include "stdafx.h"
#include "USceneComponent.h"
#include "UMesh.h"
#include "UMeshManager.h"
#include "URenderer.h"
#include "UClass.h"
#include "FBoundingBox.h"


enum class EAABBSource : uint8_t
{
    Explicit,   // SetLocalBox로 직접 세팅
    FromMesh,   // TargetMesh->PrecomputedLocalBox 사용
};

class USceneComponent;
class UMesh;

class UBoundingBoxComponent : public USceneComponent
{
    DECLARE_UCLASS(UBoundingBoxComponent, USceneComponent)
public:
    UBoundingBoxComponent();
    virtual ~UBoundingBoxComponent() = default;

    // Gizmo와 동일 인터페이스
    bool Init(UMeshManager* meshManager);

    // 어떤 대상을 둘러쌀지 지정(대상 컴포넌트의 월드 행렬을 사용)
    void SetTarget(USceneComponent* inTarget) { Target = inTarget; }

    // 로컬 AABB 소스 선택
    void SetSource(EAABBSource src) { Source = src; }
    // 1) Explicit 모드일 때: 외부에서 로컬 AABB 지정
    void SetLocalBox(const FBoundingBox& box) { LocalBox = box; }
    // 2) FromMesh 모드일 때: 대상 메시 지정(메시에 저장된 Local AABB를 사용)
    void SetTargetMesh(UMesh* inMesh) { TargetMesh = inMesh; }

    // 색상, OnTop(깊이 위에 그리기) 옵션
    void SetColor(const FVector4& rgba) { Color = rgba; }
    void SetDrawOnTop(bool b) { bDrawOnTop = b; }
    // 디버그: 마지막 계산된 월드 AABB 반환
    const FBoundingBox& GetWorldBox() const { return WorldBox; }

    // === USceneComponent 인터페이스 ===
    virtual FMatrix GetWorldTransform() override;              // 여기서는 내부 캐시된 박스 월드행렬 반환
    virtual void UpdateConstantBuffer(URenderer& renderer);
    virtual void Update(float deltaTime);
    virtual void Draw(URenderer& renderer);
    virtual void DrawOnTop(URenderer& renderer);


private:
    // 로컬AABB -> 월드AABB (Arvo 방식, row-vector 규약/LH)
    static FBoundingBox TransformAABB_Arvo(const FBoundingBox& local, const FMatrix& M_world);

    // 로컬 → Center/Extents
    static inline void AABB_CenterExtents(const FBoundingBox& b, FVector& C, FVector& E) {
        C = (b.Min + b.Max) * 0.5f;
        E = (b.Max - b.Min) * 0.5f;
    }
    static FBoundingBox TransformSphereToWorldAABB(const FVector& localCenter, float r, const FMatrix& M_world);

private:
    UMesh* meshWire = nullptr;     // [-1,1]^3 단위 큐브(WIRE)
    UMesh* TargetMesh = nullptr;   // Source==FromMesh 일 때 사용
    USceneComponent* Target = nullptr; // 위치/회전/스케일 가져올 주체

    EAABBSource Source = EAABBSource::Explicit;
    FBoundingBox LocalBox = FBoundingBox::Empty();

    // 캐시: 매 프레임 계산한 월드 AABB와 그에 대응하는 박스 월드행렬
    FBoundingBox WorldBox;
    FMatrix WBox = FMatrix::IdentityMatrix();

    FVector4 Color = FVector4(1, 1, 0, 1); // 노랑 기본
    bool bDrawOnTop = true;
};
