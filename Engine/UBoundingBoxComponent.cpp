#include "stdafx.h"
#include "UBoundingBoxComponent.h"
#include "UPrimitiveComponent.h"
#include "UClass.h"
#include "FBoundingBox.h"
// 메타 등록
IMPLEMENT_UCLASS(UBoundingBoxComponent, USceneComponent)


// ★ 스케일 추출/균등판정/동일성 헬퍼
static inline void ExtractScale_RowMajor(const FMatrix& M, float& sx, float& sy, float& sz) {
    auto len = [](float x, float y, float z) { return sqrtf(x * x + y * y + z * z); };
    sx = len(M.M[0][0], M.M[0][1], M.M[0][2]);
    sy = len(M.M[1][0], M.M[1][1], M.M[1][2]);
    sz = len(M.M[2][0], M.M[2][1], M.M[2][2]);
}
// 균등 스케일 인지?
static inline bool IsUniform(float sx, float sy, float sz, float eps = 1e-4f) {
    return fabsf(sx - sy) < eps && fabsf(sy - sz) < eps;
}
// 문자열 s에서 실수를 파싱해서 반환
static inline float ParseFloatOr(const FString& s, float defv) {
    try { return s.empty() ? defv : std::stof(s); }
    catch (...) { return defv; }
}
// 각 컬럼의 L2 길이(행벡터 규약: 열 기준)
static inline void ColumnL2_RowMajor(const FMatrix& M, float& cx, float& cy, float& cz)
{
    auto l2 = [](float a, float b, float c) { return sqrtf(a * a + b * b + c * c); };
    cx = l2(M.M[0][0], M.M[1][0], M.M[2][0]); // col 0
    cy = l2(M.M[0][1], M.M[1][1], M.M[2][1]); // col 1
    cz = l2(M.M[0][2], M.M[1][2], M.M[2][2]); // col 2
}

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
    if (!meshName.empty() || meshName[0] == '\0') meshName = "UnitCube_Wire";

    meshWire = meshManager->RetrieveMesh(meshName);
    if (!meshWire) {
        UE_LOG("[AABB] '%s' mesh not found. Building fallback wire cube.\n", meshName.c_str());
    }
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
// centerWorld = centerLocal * R + T
// Ew = (|R|)^T * halfSize   (row-vector 기준으로 구현하면 결국 각 축 절댓값을 요소곱으로 더하는 것과 동일)
FBoundingBox UBoundingBoxComponent::TransformAABB_Arvo(const FBoundingBox& local, const FMatrix& M_world)
{
    FVector centerLocal = (local.Min + local.Max) * 0.5f;
    FVector halfSize = (local.Max - local.Min) * 0.5f;

    // 회전부(R)와 이동(T)
    FMatrix R = M_world; // 상단 3x3 사용
    FVector T = FVector(M_world.M[3][0], M_world.M[3][1], M_world.M[3][2]);

    // centerWorld = centerLocal * R + T
    FVector centerWorld;
    centerWorld.X = centerLocal.X * R.M[0][0] + centerLocal.Y * R.M[1][0] + centerLocal.Z * R.M[2][0] + T.X;
    centerWorld.Y = centerLocal.X * R.M[0][1] + centerLocal.Y * R.M[1][1] + centerLocal.Z * R.M[2][1] + T.Y;
    centerWorld.Z = centerLocal.X * R.M[0][2] + centerLocal.Y * R.M[1][2] + centerLocal.Z * R.M[2][2] + T.Z;

    // |R|
    float A[3][3]; Abs3x3_RowMajor(R, A);

    // lengthWorld = (|R|)^T * halfSize  ==> 각 성분: sum_i (A[i][axis] * halfSize_i)
    FVector lengthWorld;
    lengthWorld.X = halfSize.X * A[0][0] + halfSize.Y * A[1][0] + halfSize.Z * A[2][0];
    lengthWorld.Y = halfSize.X * A[0][1] + halfSize.Y * A[1][1] + halfSize.Z * A[2][1];
    lengthWorld.Z = halfSize.X * A[0][2] + halfSize.Y * A[1][2] + halfSize.Z * A[2][2];

    FBoundingBox out;
    out.Min = centerWorld - lengthWorld;
    out.Max = centerWorld + lengthWorld;
    return out;
}
FBoundingBox UBoundingBoxComponent::TransformSphereToWorldAABB(const FVector& centerLocal, float r, const FMatrix& M_world)
{
    // 월드 중심: Cw = centerLocal * R + T  (행벡터 규약)
    const FMatrix& L = M_world; // 상단 3x3 선형부
    const FVector  T(M_world.M[3][0], M_world.M[3][1], M_world.M[3][2]);

    FVector centerWorld;
    centerWorld.X = centerLocal.X * L.M[0][0] + centerLocal.Y * L.M[1][0] + centerLocal.Z * L.M[2][0] + T.X;
    centerWorld.Y = centerLocal.X * L.M[0][1] + centerLocal.Y * L.M[1][1] + centerLocal.Z * L.M[2][1] + T.Y;
    centerWorld.Z = centerLocal.X * L.M[0][2] + centerLocal.Y * L.M[1][2] + centerLocal.Z * L.M[2][2] + T.Z;

    // ★ 핵심: 타원체의 각 축 반경 = r * ||col_j(L)||_2
    float lengthX, lengthY, lengthZ;
    ColumnL2_RowMajor(L, lengthX, lengthY, lengthZ);
    const FVector lengthWorld(r * lengthX, r * lengthY, r * lengthZ);

    FBoundingBox out;
    out.Min = centerWorld - lengthWorld;
    out.Max = centerWorld + lengthWorld;
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

    const FMatrix M_target = Target->GetWorldTransform();

    // 기본값(박스)
    FBoundingBox srcLocal = LocalBox;
    bool  bUseSphere = false;
    float sphereR = 0.0f;
    FVector sphereCenterLocal(0, 0, 0);

    // 1) 컴포넌트 클래스 메타 읽기
    const FString boundsType = Target->GetClass()->GetMeta("BoundsType");
    const bool metaSaysSphere = (!boundsType.empty() && (boundsType == "Sphere" || boundsType == "sphere"));

    // 2) FromMesh 소스일 때 메시 참조
    UMesh* mesh = TargetMesh;
    if (Source == EAABBSource::FromMesh && !mesh) {
        if (auto prim = Target->Cast<UPrimitiveComponent>()) mesh = prim->GetMesh();
    }

    if (metaSaysSphere)
    {
        // === Sphere 경로 ===
        bUseSphere = true;

        // 반지름: 메타 우선
        sphereR = ParseFloatOr(Target->GetClass()->GetMeta("BoundsRadius"), 0.0f);

        if (mesh && mesh->HasPrecomputedAABB()) {
            FVector C, H; FBoundingBox::CenterExtents(mesh->GetPrecomputedLocalBox(), C, H);
            sphereCenterLocal = C;
            if (sphereR <= 0.0f) sphereR = max(H.X, max(H.Y, H.Z)); // 보수적
        }
        else {
            // 메시 AABB가 없으면 LocalBox에서 추정(Explicit 모드 포함)
            FVector C, H; FBoundingBox::CenterExtents(LocalBox, C, H);
            sphereCenterLocal = C;
            if (sphereR <= 0.0f) sphereR = max(H.X, max(H.Y, H.Z));
            if (sphereR <= 0.0f) { sphereCenterLocal = FVector(0, 0, 0); sphereR = 1.0f; } // 최후 fallback
        }

        WorldBox = TransformSphereToWorldAABB(sphereCenterLocal, sphereR, M_target);
    }
    else
    {
        // === Box(Arvo) 경로 ===
        if (Source == EAABBSource::FromMesh && mesh && mesh->HasPrecomputedAABB())
            srcLocal = mesh->GetPrecomputedLocalBox();
        else if (Source == EAABBSource::FromMesh && (!mesh || !mesh->HasPrecomputedAABB())) {
            // 안전망
            srcLocal.Min = FVector(-0.5f, -0.5f, -0.5f);
            srcLocal.Max = FVector(+0.5f, +0.5f, +0.5f);
        }
        // Explicit일 땐 위에서 기본값으로 LocalBox가 이미 들어가 있음

        WorldBox = TransformAABB_Arvo(srcLocal, M_target);
    }

    // 3) 월드 AABB → 박스 행렬
    FVector C, H; FBoundingBox::CenterExtents(WorldBox, C, H);
    const FMatrix S = FMatrix::Scale(H);
    const FMatrix T = FMatrix::TranslationRow(C);
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
