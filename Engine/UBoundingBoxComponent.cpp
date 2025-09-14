#include "stdafx.h"
#include "UBoundingBoxComponent.h"
#include "UPrimitiveComponent.h"
#include "UClass.h"
#include "BoundingBox.h"
// IsA를 통한 구별법을 위해 헤더 추가
#include "UQuadComponent.h"
// 메타 등록
IMPLEMENT_UCLASS(UBoundingBoxComponent, USceneComponent)

// ★ 스케일 추출/균등판정/동일성 헬퍼
static inline void ExtractScaleRowMajor(const FMatrix& Matrix, float& ScaleX, float& ScaleY, float& ScaleZ) {
    auto len = [](float X, float Y, float Z) { return sqrtf(X * X + Y * Y + Z * Z); };
    ScaleX = len(Matrix.M[0][0], Matrix.M[0][1], Matrix.M[0][2]);
    ScaleY = len(Matrix.M[1][0], Matrix.M[1][1], Matrix.M[1][2]);
    ScaleZ = len(Matrix.M[2][0], Matrix.M[2][1], Matrix.M[2][2]);
}
// 균등 스케일 인지?
static inline bool IsUniform(float ScaleX, float ScaleY, float ScaleZ, float Epsilon = 1e-4f) {
    return fabsf(ScaleX - ScaleY) < Epsilon && fabsf(ScaleY - ScaleZ) < Epsilon;
}
// 문자열 s에서 실수를 파싱해서 반환
static inline float ParseNameToFloat(const FString& Name, float defv) {
    try { return Name.empty() ? defv : std::stof(Name); }
    catch (...) { return defv; }
}
// 각 컬럼의 길이(행벡터 규약: 열 기준)
static inline void ComputeColumnScales(const FMatrix& Matrix, float& ScaleX, float& ScaleY, float& ScaleZ)
{
    auto l2 = [](float X, float Y, float Z) { return sqrtf(X * X + Y * Y + Z * Z); };
    ScaleX = l2(Matrix.M[0][0], Matrix.M[1][0], Matrix.M[2][0]); // col 0
    ScaleY = l2(Matrix.M[0][1], Matrix.M[1][1], Matrix.M[2][1]); // col 1
    ScaleZ = l2(Matrix.M[0][2], Matrix.M[1][2], Matrix.M[2][2]); // col 2
}
UBoundingBoxComponent::UBoundingBoxComponent()
{
    // 기본 로컬 AABB는 단위 큐브 정도로 초기화해도 무방(외부에서 세팅 가정)
    LocalBox.Min = FVector(-0.5f, -0.5f, -0.5f);
    LocalBox.Max = FVector(+0.5f, +0.5f, +0.5f);
}

bool UBoundingBoxComponent::Init(UMeshManager* MeshManager)
{
    if (!MeshManager) return false;

    // 메타에서 메시 이름을 받아오되 기본값은 "UnitCube_Wire"
    // TODO - Font의 기반이 될 Quad는 AABB를 받지 않도록 수정해야함.
    FString MeshName = GetClass()->GetMeta("MeshName");
    if (MeshName.empty() || MeshName[0] == '\0') MeshName = "UnitCube_Wire";

    MeshWire = MeshManager->RetrieveMesh(MeshName);
    if (!MeshWire) {
        UE_LOG("[AABB] '%s' mesh not found. Building fallback wire cube.\n", MeshName.c_str());
    }
    return MeshWire != nullptr;
}

// 행벡터 규약: R의 "행"이 축 벡터
static inline void Abs3x3RowMajor(const FMatrix& M, float A[3][3])
{
    A[0][0] = fabsf(M.M[0][0]); A[0][1] = fabsf(M.M[0][1]); A[0][2] = fabsf(M.M[0][2]);
    A[1][0] = fabsf(M.M[1][0]); A[1][1] = fabsf(M.M[1][1]); A[1][2] = fabsf(M.M[1][2]);
    A[2][0] = fabsf(M.M[2][0]); A[2][1] = fabsf(M.M[2][1]); A[2][2] = fabsf(M.M[2][2]);
}

// row-vector, LH 가정.
// MatrixWorld = [ R | T ]
// CenterWorld = CenterLocal * R + T
// Ew = (|R|)^T * halfSize   (row-vector 기준으로 구현하면 결국 각 축 절댓값을 요소곱으로 더하는 것과 동일)
FBoundingBox UBoundingBoxComponent::TransformArvoAABB(const FBoundingBox& Local, const FMatrix& MatrixWorld)
{
    FVector CenterLocal = (Local.Min + Local.Max) * 0.5f;
    FVector halfSize = (Local.Max - Local.Min) * 0.5f;

    // 회전부(R)와 이동(T)
    FMatrix R = MatrixWorld; // 상단 3x3 사용
    FVector T = FVector(MatrixWorld.M[3][0], MatrixWorld.M[3][1], MatrixWorld.M[3][2]);

    // CenterWorld = CenterLocal * R + T
    FVector CenterWorld;
    CenterWorld.X = CenterLocal.X * R.M[0][0] + CenterLocal.Y * R.M[1][0] + CenterLocal.Z * R.M[2][0] + T.X;
    CenterWorld.Y = CenterLocal.X * R.M[0][1] + CenterLocal.Y * R.M[1][1] + CenterLocal.Z * R.M[2][1] + T.Y;
    CenterWorld.Z = CenterLocal.X * R.M[0][2] + CenterLocal.Y * R.M[1][2] + CenterLocal.Z * R.M[2][2] + T.Z;

    // |R|
    float A[3][3]; Abs3x3RowMajor(R, A);

    // ExtentWorld = (|R|)^T * halfSize  ==> 각 성분: sum_i (A[i][axis] * halfSize_i)
    FVector ExtentWorld;
    ExtentWorld.X = halfSize.X * A[0][0] + halfSize.Y * A[1][0] + halfSize.Z * A[2][0];
    ExtentWorld.Y = halfSize.X * A[0][1] + halfSize.Y * A[1][1] + halfSize.Z * A[2][1];
    ExtentWorld.Z = halfSize.X * A[0][2] + halfSize.Y * A[1][2] + halfSize.Z * A[2][2];

    FBoundingBox out;
    out.Min = CenterWorld - ExtentWorld;
    out.Max = CenterWorld + ExtentWorld;
    return out;
}
FBoundingBox UBoundingBoxComponent::TransformSphereToWorldAABB(const FVector& CenterLocal, float Radius, const FMatrix& MatrixWorld)
{
    // 월드 중심: CenterWorld = CenterLocal * Rotation + Translation  (행벡터 규약)
    const FVector  Translation(MatrixWorld.M[3][0], MatrixWorld.M[3][1], MatrixWorld.M[3][2]);

    FVector CenterWorld;
    CenterWorld.X = CenterLocal.X * MatrixWorld.M[0][0] + CenterLocal.Y * MatrixWorld.M[1][0] + CenterLocal.Z * MatrixWorld.M[2][0] + Translation.X;
    CenterWorld.Y = CenterLocal.X * MatrixWorld.M[0][1] + CenterLocal.Y * MatrixWorld.M[1][1] + CenterLocal.Z * MatrixWorld.M[2][1] + Translation.Y;
    CenterWorld.Z = CenterLocal.X * MatrixWorld.M[0][2] + CenterLocal.Y * MatrixWorld.M[1][2] + CenterLocal.Z * MatrixWorld.M[2][2] + Translation.Z;

    // ★ 핵심: 타원체의 각 축 반경 = r * ||col_j(L)||_2
    float lengthX, lengthY, lengthZ;
    ComputeColumnScales(MatrixWorld, lengthX, lengthY, lengthZ);
    const FVector ExtentWorld(Radius * lengthX, Radius * lengthY, Radius * lengthZ);

    FBoundingBox Out;
    Out.Min = CenterWorld - ExtentWorld;
    Out.Max = CenterWorld + ExtentWorld;
    return Out;
}
FMatrix UBoundingBoxComponent::GetWorldTransform()
{
    // 이 컴포넌트는 계층 회전/스케일을 "상속"하지 않고,
    // 계산된 월드 AABB에 맞춘 박스 월드행렬 WorldBoxMatrix를 그대로 반환
    return WorldBoxMatrix;
}

void UBoundingBoxComponent::UpdateConstantBuffer(URenderer& Renderer)
{
    // Gizmo와 동일 API 사용: 모델행렬 + 색상 + 선택여부
    // 선택여부는 필요 없으니 false
    Renderer.SetModel(WorldBoxMatrix, Color, /*bIsSelected=*/false);
}

void UBoundingBoxComponent::Update(float /*deltaTime*/)
{
    if (!Target) return;
    // 숨김 플래그 초기화
    bHideAABB = false;
    /*
        USceneComponent 중 AABB를 보이지 않게 처리하는 두 가지 방법
        TODO - 추후에 IsA로 변경 시 해당 컴포넌트 헤더 추가
        매니저에서 처리하도록 하는 것도 해봤는데
        도형을 눌렀다가 Quad를 누르면 이전 도형에서 AABB 보이는 문제 발생
        따라서 우선은 컴포넌트에서 처리하기로
    */
    // 1. 타입으로 필터(가장 견고)
    if (Target->IsA<UQuadComponent>())
    {
        bHideAABB = true;
        return; // 그릴 필요 없으므로 추가 계산 생략
    }
    // 2. 타깃 컴포넌트의 클래스 메타로 판정(유연함)
    const std::string TargetClassMeshName = Target->GetClass()->GetMeta("MeshName");
    if (TargetClassMeshName == "Quad")
    {
        bHideAABB = true;
        return; // 그릴 필요 없으니 추가 계산 생략
    }


    const FMatrix MeshTarget = Target->GetWorldTransform();

    // 기본값(박스)
    FBoundingBox SrcLocal = LocalBox;
    bool  bUseSphere = false;
    float SphereRadius = 0.0f;
    FVector SphereCenterLocal(0, 0, 0);

    // 1) 컴포넌트 클래스 메타 읽기
    const FString BoundsType = Target->GetClass()->GetMeta("BoundsType");
    const bool MetaSaysSphere = (!BoundsType.empty() && (BoundsType == "Sphere" || BoundsType == "sphere"));

    // 2) FromMesh 소스일 때 메시 참조
    UMesh* Mesh = TargetMesh;
    if (Source == EAABBSource::FromMesh && !Mesh) {
        if (auto Prim = Target->Cast<UPrimitiveComponent>()) Mesh = Prim->GetMesh();
    }

    if (MetaSaysSphere)
    {
        // === Sphere 경로 ===
        bUseSphere = true;

        // 반지름: 메타 우선
        SphereRadius = ParseNameToFloat(Target->GetClass()->GetMeta("BoundsRadius"), 0.0f);

        if (Mesh && Mesh->HasPrecomputedAABB()) {
            FVector Center, Half; FBoundingBox::CenterExtents(Mesh->GetPrecomputedLocalBox(), Center, Half);
            SphereCenterLocal = Center;
            if (SphereRadius <= 0.0f) SphereRadius = max(Half.X, max(Half.Y, Half.Z)); // 보수적
        }
        else {
            // 메시 AABB가 없으면 LocalBox에서 추정(Explicit 모드 포함)
            FVector Center, Half; FBoundingBox::CenterExtents(LocalBox, Center, Half);
            SphereCenterLocal = Center;
            if (SphereRadius <= 0.0f) SphereRadius = max(Half.X, max(Half.Y, Half.Z));
            if (SphereRadius <= 0.0f) { SphereCenterLocal = FVector(0, 0, 0); SphereRadius = 1.0f; } // 최후 fallback
        }

        WorldBox = TransformSphereToWorldAABB(SphereCenterLocal, SphereRadius, MeshTarget);
    }
    else
    {
        // === Box(Arvo) 경로 ===
        if (Source == EAABBSource::FromMesh && Mesh && Mesh->HasPrecomputedAABB())
            SrcLocal = Mesh->GetPrecomputedLocalBox();
        else if (Source == EAABBSource::FromMesh && (!Mesh || !Mesh->HasPrecomputedAABB())) {
            // 안전망
            SrcLocal.Min = FVector(-0.5f, -0.5f, -0.5f);
            SrcLocal.Max = FVector(+0.5f, +0.5f, +0.5f);
        }
        // Explicit일 땐 위에서 기본값으로 LocalBox가 이미 들어가 있음

        WorldBox = TransformArvoAABB(SrcLocal, MeshTarget);
    }

    // 3) 월드 AABB → 박스 행렬
    FVector Center, Half; FBoundingBox::CenterExtents(WorldBox, Center, Half);
    const FMatrix S = FMatrix::Scale(Half);
    const FMatrix T = FMatrix::TranslationRow(Center);
    WorldBoxMatrix = S * T;
}

void UBoundingBoxComponent::Draw(URenderer& Renderer)
{
    // AABB를 숨긴다면
    if (bHideAABB) return;
    if (!MeshWire || !MeshWire->VertexBuffer) return;
    // UpdateConstantBuffer(Renderer);
    Renderer.SubmitLineList(MeshWire->Vertices, MeshWire->Indices, WorldBoxMatrix);
}

void UBoundingBoxComponent::DrawOnTop(URenderer& Renderer)
{
    // AABB를 숨긴다면
    if (bHideAABB) return;
    if (!MeshWire || !MeshWire->VertexBuffer) return;
    UpdateConstantBuffer(Renderer);
    Renderer.DrawMeshOnTop(MeshWire);
}
