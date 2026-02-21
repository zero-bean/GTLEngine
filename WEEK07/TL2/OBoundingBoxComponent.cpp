#include"pch.h"
//#include "OBoundingBoxComponent.h"
//#include "Vector.h"
//#include "AABoundingBoxComponent.h"
//
//std::vector<FVector> MakeVerticesFromFBox(const FBox& Box)
//{
//    const FVector& Min = Box.Min;
//    const FVector& Max = Box.Max;
//
//    return {
//        {Min.X, Min.Y, Min.Z},
//        {Max.X, Min.Y, Min.Z},
//        {Min.X, Max.Y, Min.Z},
//        {Max.X, Max.Y, Min.Z},
//        {Min.X, Min.Y, Max.Z},
//        {Max.X, Min.Y, Max.Z},
//        {Min.X, Max.Y, Max.Z},
//        {Max.X, Max.Y, Max.Z}
//    };
//}
//UOBoundingBoxComponent::UOBoundingBoxComponent()
//    : LocalMin(FVector{}), LocalMax(FVector{}), LineColor(1.0f, 1.0f, 0.0f, 1.0f)
//{
//
//}
//
//UOBoundingBoxComponent::~UOBoundingBoxComponent()
//{
//}
//
//void UOBoundingBoxComponent::SetFromVertices(const std::vector<FVector>& Verts)
//{
//    if (Verts.empty()) return;
//
//    LocalMin = LocalMax = Verts[0];
//    for (auto& v : Verts)
//    {
//        LocalMin = LocalMin.ComponentMin(v);
//        LocalMax = LocalMax.ComponentMax(v);
//    }
//}
//
//
//FVector UOBoundingBoxComponent::GetExtent() const
//{
//    return (LocalMax - LocalMin) * 0.5f;
//}
//
//std::vector<FVector> UOBoundingBoxComponent::GetLocalCorners() const
//{
//    return {
//        {LocalMin.X, LocalMin.Y, LocalMin.Z},
//        {LocalMax.X, LocalMin.Y, LocalMin.Z},
//        {LocalMin.X, LocalMax.Y, LocalMin.Z},
//        {LocalMax.X, LocalMax.Y, LocalMin.Z},
//        {LocalMin.X, LocalMin.Y, LocalMax.Z},
//        {LocalMax.X, LocalMin.Y, LocalMax.Z},
//        {LocalMin.X, LocalMax.Y, LocalMax.Z},
//        {LocalMax.X, LocalMax.Y, LocalMax.Z}
//    };
//}
//TArray<FVector> UOBoundingBoxComponent::GetWorldCorners() const
//{
//    FMatrix WorldMat = FMatrix::Identity();
//    if (GetOwner()) {
//        WorldMat = GetOwner()->GetWorldMatrix();
//    }
//
//    std::vector<FVector> LocalCorners = GetLocalCorners();
//    TArray<FVector> WorldCorners(8);
//    int idx = 0;
//    for (FVector& LocalCorner : LocalCorners)
//    {
//        FVector4 Vt4 = FVector4(LocalCorner, 1);
//        Vt4 = Vt4 * WorldMat;
//        WorldCorners[idx++] = FVector(Vt4.X, Vt4.Y, Vt4.Z);
//    }
//    return WorldCorners;
//}
//
//
//FBox UOBoundingBoxComponent::GetWorldOBBFromAttachParent() const
//{
//	
//    if (!AttachParent) return FBox();
//
//    // AttachParent의 로컬 코너들
//    auto corners = GetLocalCorners();
//
//    // 월드 변환된 첫 번째 점으로 초기화
//    FVector MinW = AttachParent->GetWorldTransform().TransformPosition(corners[0]);
//    FVector MaxW = MinW;
//
//    for (auto& c : corners)
//    {
//        FVector wc = AttachParent->GetWorldTransform().TransformPosition(c);
//        MinW = MinW.ComponentMin(wc);
//        MaxW = MaxW.ComponentMax(wc);
//    }
//    //BBWorldMatrix
//    return FBox(MinW, MaxW);
//}
//
//void UOBoundingBoxComponent::Render(URenderer* Renderer, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix)
//{
//    // LocalMin과 LocalMax가 유효하지 않으면 렌더링하지 않음
//    if (LocalMin == FVector{} && LocalMax == FVector{})
//        return;
//
//    TArray<FVector> OBBStart;
//    TArray<FVector> OBBEnd;
//    TArray<FVector4> OBBColor;
//
//    // 로컬 코너들을 월드 공간으로 변환
//    auto localCorners = GetLocalCorners();
//    FMatrix WorldMat = FMatrix::Identity();
//    if (GetOwner()) {
//        WorldMat = GetOwner()->GetWorldMatrix();
//    }
//
//    // 8개의 월드 코너 계산
//    std::vector<FVector> worldCorners;
//    worldCorners.reserve(8);
//    for (const auto& corner : localCorners) {
//        FVector4 worldCorner = FVector4(corner.X, corner.Y, corner.Z, 1.0f) * WorldMat;
//        worldCorners.push_back(FVector(worldCorner.X, worldCorner.Y, worldCorner.Z));
//    }
//
//    // 안전성 체크
//    if (worldCorners.size() != 8)
//        return;
//
//    // OBB는 회전된 박스이므로 8개 코너를 직접 연결
//    // 인덱스: 0(min,min,min), 1(max,min,min), 2(min,max,min), 3(max,max,min)
//    //         4(min,min,max), 5(max,min,max), 6(min,max,max), 7(max,max,max)
//
//	CreateLineData(worldCorners,  OBBStart, OBBEnd, OBBColor);
//
//    Renderer->AddLines(OBBStart, OBBEnd, OBBColor);
//}
//
//void UOBoundingBoxComponent::CreateLineData(
//    const std::vector<FVector>& corners,
//    OUT TArray<FVector>& Start,
//    OUT TArray<FVector>& End,
//    OUT TArray<FVector4>& Color)
//{
//    // 8개 꼭짓점 정의
//    static const int edges[12][2] = {
//         {0,1}, {1,3}, {3,2}, {2,0}, // 아래
//         {4,5}, {5,7}, {7,6}, {6,4}, // 위
//         {0,4}, {1,5}, {2,6}, {3,7}  // 기둥
//    };
//
//    for (int i = 0; i < 12; ++i)
//    {
//        Start.Add(corners[edges[i][0]]);
//        End.Add(corners[edges[i][1]]);
//        Color.Add(LineColor);
//    }
//}
//
//FOrientedBound UOBoundingBoxComponent::GetWorldOrientedBound() const
//{
//    FMatrix WorldMat = FMatrix::Identity();
//    if (GetOwner()) {
//        WorldMat = GetOwner()->GetWorldMatrix();
//    }
//
//    FVector LocalCenter = (LocalMin + LocalMax) * 0.5f;
//    FVector LocalExtents = (LocalMax - LocalMin) * 0.5f;
//
//    FVector4 WorldCenter4 = FVector4(LocalCenter.X, LocalCenter.Y, LocalCenter.Z, 1.0f) * WorldMat;
//    FVector WorldCenter = FVector(WorldCenter4.X, WorldCenter4.Y, WorldCenter4.Z);
//
//    FVector WorldExtents;
//    WorldExtents.X = std::abs(WorldMat.M[0][0] * LocalExtents.X) +
//                     std::abs(WorldMat.M[0][1] * LocalExtents.Y) +
//                     std::abs(WorldMat.M[0][2] * LocalExtents.Z);
//    WorldExtents.Y = std::abs(WorldMat.M[1][0] * LocalExtents.X) +
//                     std::abs(WorldMat.M[1][1] * LocalExtents.Y) +
//                     std::abs(WorldMat.M[1][2] * LocalExtents.Z);
//    WorldExtents.Z = std::abs(WorldMat.M[2][0] * LocalExtents.X) +
//                     std::abs(WorldMat.M[2][1] * LocalExtents.Y) +
//                     std::abs(WorldMat.M[2][2] * LocalExtents.Z);
//
//    return FOrientedBound(WorldCenter, WorldExtents, WorldMat);
//}
//
//bool UOBoundingBoxComponent::RayIntersectsOBB(const FVector& Origin, const FVector& Direction, float& Distance) const
//{
//    FOrientedBound WorldOBB = GetWorldOrientedBound();
//    return WorldOBB.RayIntersects(Origin, Direction, Distance);
//}
//
//UObject* UOBoundingBoxComponent::Duplicate()
//{
//    // 부모 클래스 패턴을 따라서 NewObject로 생성 (생성자 호출)
//    UOBoundingBoxComponent* DuplicatedComponent = Cast<UOBoundingBoxComponent>(NewObject(GetClass()));
//
//    // 부모 클래스(USceneComponent)의 공통 속성 복사
//    CopyCommonProperties(DuplicatedComponent);
//
//    // OBoundingBoxComponent 고유 속성 복사
//    DuplicatedComponent->LocalMin = this->LocalMin;
//    DuplicatedComponent->LocalMax = this->LocalMax;
//    DuplicatedComponent->LineColor = this->LineColor;
//    // 자식 컴포넌트 복제
//    DuplicatedComponent->DuplicateSubObjects();
//
//    return DuplicatedComponent;
//}
//bool UOBoundingBoxComponent::IntersectWithAABB(const FBound& AABB) const
//{
//    FMatrix WorldMatrix = GetOwner()->GetWorldMatrix();
//
//    FVector WorldAxis[3];
//    WorldAxis[0] = FVector(1, 0, 0) * WorldMatrix;
//    WorldAxis[1] = FVector(0, 1, 0) * WorldMatrix;
//    WorldAxis[2] = FVector(0, 0, 1) * WorldMatrix;
//    TArray<FVector> WorldCorners = GetWorldCorners();
//    FVector SATAxises[15];
//    SATAxises[0] = FVector(1, 0, 0);
//    SATAxises[1] = FVector(0, 1, 0);
//    SATAxises[2] = FVector(0, 0, 1);
//    SATAxises[3] = WorldAxis[0];
//    SATAxises[4] = WorldAxis[1];
//    SATAxises[5] = WorldAxis[2];
//
//    for (int i = 0; i < 3; i++)
//    {
//        SATAxises[6 + i * 3] = FVector::Cross(WorldAxis[i], FVector(1, 0, 0));
//        SATAxises[6 + i * 3 + 1] = FVector::Cross(WorldAxis[i], FVector(0, 1, 0));
//        SATAxises[6 + i * 3 + 2] = FVector::Cross(WorldAxis[i], FVector(0, 0, 1));
//    }
//
//    for (FVector& SATAxis : SATAxises)
//    {
//        FVector Normal = SATAxis.GetNormalized();
//        if (Normal == FVector(0, 0, 0))
//        {
//            continue;
//        }
//
//        //AABB Min, Max 구하기
//        //한쪽 끝점과 반대쪽 끝점
//        FVector AABBOuter1 = FVector(SATAxis.X > 0 ? AABB.Min.X : AABB.Max.X, SATAxis.Y > 0 ? AABB.Min.Y : AABB.Max.Y, SATAxis.Z > 0 ? AABB.Min.Z : AABB.Max.Z);
//        FVector AABBOuter2 = FVector(SATAxis.X > 0 ? AABB.Max.X : AABB.Min.X, SATAxis.Y > 0 ? AABB.Max.Y : AABB.Min.Y, SATAxis.Z > 0 ? AABB.Max.Z : AABB.Min.Z);
//        float AABBDotValue1 = Normal.Dot(AABBOuter1);
//        float AABBDotValue2 = Normal.Dot(AABBOuter2);
//        float AABBMin = AABBDotValue1 < AABBDotValue2 ? AABBDotValue1 : AABBDotValue2;
//        float AABBMax = AABBDotValue1 < AABBDotValue2 ? AABBDotValue2 : AABBDotValue1;
//
//        //OBB Min, Max 구하기
//        float OBBMin = 0, OBBMax = 0;
//        FVector OBBVertex = WorldCorners[0];
//        float OBBVertexDotValue = Normal.Dot(OBBVertex);
//        OBBMin = OBBVertexDotValue;
//        OBBMax = OBBVertexDotValue;
//        for (int i = 1; i < 8; i++)
//        {
//            OBBVertex = WorldCorners[i];
//            OBBVertexDotValue = Normal.Dot(OBBVertex);
//            if (OBBMin > OBBVertexDotValue)
//            {
//                OBBMin = OBBVertexDotValue;
//            }
//            else if (OBBMax < OBBVertexDotValue)
//            {
//                OBBMax = OBBVertexDotValue;
//            }
//        }
//
//        //A의 최대가 B의 최소보다 작거나, B의 최대가 A의 최소보다 작으면 겹치지 않음
//        if (AABBMax < OBBMin || OBBMax < AABBMin)
//        {
//            //한 축이라도 겹치지 않으면 충돌 x
//            return false;
//        }
//    }
//    return true;
//
//}
