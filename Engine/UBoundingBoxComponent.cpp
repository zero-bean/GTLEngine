#include "stdafx.h"
#include "UBoundingBoxComponent.h"
#include "UClass.h"

// 메타 등록
IMPLEMENT_UCLASS(UBoundingBoxComponent, USceneComponent)

UBoundingBoxComponent::UBoundingBoxComponent()
{
    // 기본 로컬 AABB는 단위 큐브 정도로 초기화해도 무방(외부에서 세팅 가정)
    LocalBox.Min = FVector(-0.5f, -0.5f, -0.5f);
    LocalBox.Max = FVector(+0.5f, +0.5f, +0.5f);
}

bool UBoundingBoxComponent::Init(UMeshManager* meshManager)
{
    if (!meshManager) return false;

    // 메타에서 메시 이름을 받아오되 기본값은 "UnitCube_Wire"
    FString meshName = GetClass()->GetMeta("MeshName");
    if (meshName.empty() || meshName[0] == '\0') meshName = "UnitCube_Wire";

    meshWire = meshManager->RetrieveMesh(meshName);
    return meshWire != nullptr;
}

// 행벡터 규약: R의 "행"이 축 벡터
static inline void Abs3x3_RowMajor(const FMatrix& M, float A[3][3])
{
    A[0][0] = fabsf(M.M[0][0]); A[0][1] = fabsf(M.M[0][1]); A[0][2] = fabsf(M.M[0][2]);
    A[1][0] = fabsf(M.M[1][0]); A[1][1] = fabsf(M.M[1][1]); A[1][2] = fabsf(M.M[1][2]);
    A[2][0] = fabsf(M.M[2][0]); A[2][1] = fabsf(M.M[2][1]); A[2][2] = fabsf(M.M[2][2]);
}

// row-vector, LH 가정.
// M_world = [ R | T ]
// Cw = Cl * R + T
// Ew = (|R|)^T * El   (row-vector 기준으로 구현하면 결국 각 축 절댓값을 요소곱으로 더하는 것과 동일)
FBoundingBox UBoundingBoxComponent::TransformAABB_Arvo(const FBoundingBox& local, const FMatrix& M_world)
{
    FVector Cl = (local.Min + local.Max) * 0.5f;
    FVector El = (local.Max - local.Min) * 0.5f;

    // 회전부(R)와 이동(T)
    FMatrix R = M_world; // 상단 3x3 사용
    FVector T = FVector(M_world.M[3][0], M_world.M[3][1], M_world.M[3][2]);

    // Cw = Cl * R + T
    FVector Cw;
    Cw.X = Cl.X * R.M[0][0] + Cl.Y * R.M[1][0] + Cl.Z * R.M[2][0] + T.X;
    Cw.Y = Cl.X * R.M[0][1] + Cl.Y * R.M[1][1] + Cl.Z * R.M[2][1] + T.Y;
    Cw.Z = Cl.X * R.M[0][2] + Cl.Y * R.M[1][2] + Cl.Z * R.M[2][2] + T.Z;

    // |R|
    float A[3][3]; Abs3x3_RowMajor(R, A);

    // Ew = (|R|)^T * El  ==> 각 성분: sum_i (A[i][axis] * El_i)
    FVector Ew;
    Ew.X = El.X * A[0][0] + El.Y * A[1][0] + El.Z * A[2][0];
    Ew.Y = El.X * A[0][1] + El.Y * A[1][1] + El.Z * A[2][1];
    Ew.Z = El.X * A[0][2] + El.Y * A[1][2] + El.Z * A[2][2];

    FBoundingBox out;
    out.Min = Cw - Ew;
    out.Max = Cw + Ew;
    return out;
}

FMatrix UBoundingBoxComponent::GetWorldTransform()
{
    // 이 컴포넌트는 계층 회전/스케일을 "상속"하지 않고,
    // 계산된 월드 AABB에 맞춘 박스 월드행렬 WBox를 그대로 반환
    return WBox;
}

void UBoundingBoxComponent::UpdateConstantBuffer(URenderer& renderer)
{
    // Gizmo와 동일 API 사용: 모델행렬 + 색상 + 선택여부
    // 선택여부는 필요 없으니 false
    renderer.SetModel(WBox, Color, /*bIsSelected=*/false);
}

void UBoundingBoxComponent::Update(float /*deltaTime*/)
{
    if (!Target) return;

    // 대상의 월드 행렬
    // (USceneComponent에 유사한 GetWorldTransform()이 있다고 가정)
    FMatrix M_target = Target->GetWorldTransform();

    // 사용할 로컬 AABB 선택
    FBoundingBox srcLocal = LocalBox;
    //if (Source == EAABBSource::FromMesh && TargetMesh && TargetMesh->HasPrecomputedAABB) {
    //    srcLocal = TargetMesh->PrecomputedLocalBox; // 엔진 사양에 맞게 멤버명 조정
    //}

    // 월드 AABB 계산
    WorldBox = TransformAABB_Arvo(srcLocal, M_target);

    // 월드 AABB → Center/Extents
    FVector C, E; AABB_CenterExtents(WorldBox, C, E);

    // [-1,1]^3 단위 큐브 기준: Scale = Extents, Translate = Center
    // (행벡터 규약: 점 * S * T * V * P 로 쓰므로 S→T 순서)
    FMatrix S = FMatrix::Scale(E);
    FMatrix T = FMatrix::TranslationRow(C);
    WBox = S * T;
}

void UBoundingBoxComponent::Draw(URenderer& renderer)
{
    if (!meshWire || !meshWire->VertexBuffer) return;
    UpdateConstantBuffer(renderer);

    if (bDrawOnTop)
        renderer.DrawMeshOnTop(meshWire); // 깊이 무시(혹은 offset)로 표시
    else
        renderer.DrawMesh(meshWire);
}

void UBoundingBoxComponent::DrawOnTop(URenderer& renderer)
{
    if (!meshWire || !meshWire->VertexBuffer) return;
    UpdateConstantBuffer(renderer);
    renderer.DrawMeshOnTop(meshWire);
}
