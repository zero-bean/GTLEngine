#include "pch.h"
#include "BVH.h"
#include "StaticMeshActor.h"
#include "Picking.h"
#include "TimeProfile.h"
#include "UI/GlobalConsole.h"
#include <algorithm>
#include <cfloat>


FBVH::FBVH() : MaxDepth(0)
{
}

FBVH::~FBVH()
{
    Clear();
}

void FBVH::Build(const TArray<AActor*>& Actors)
{
    TIME_PROFILE(BVHBuild)
    Clear();

    if (Actors.Num() == 0)
        return;

    //전체 액터 순회하며 StaticMeshComponent 추출
    TArray<UPrimitiveComponent*> Primitives;
    for (const AActor* Actor : Actors)
    {
        if (!Actor || Actor->GetActorHiddenInGame())
        {
            continue;
        }

        for (auto Component : Actor->GetComponents())
        {
            if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
            {
                Primitives.Push(Primitive);
            }
        }
    }

    if (Primitives.Num() == 0)
    {
        return;
    }
    Build(Primitives);
}

void FBVH::Build(const TArray<UPrimitiveComponent*>& Primitives)
{
    Clear();
    TStatId BVHBuildStatId;
    FScopeCycleCounter BVHBuildTimer(BVHBuildStatId);
    // 1. 액터들의 AABB 정보 수집
    PrimitiveBounds.Reserve(Primitives.Num());
    PrimitiveIndices.Reserve(Primitives.Num());

    for (int i = 0; i < Primitives.Num(); ++i)
    {
        UPrimitiveComponent* Primitive = Primitives[i];
        const FAABB* PrimitiveBounds_Local = nullptr;
        bool bHasBounds = false;

        PrimitiveBounds.emplace_back(FBVHPtimitive(Primitive, Primitive->GetWorldAABB()));
        PrimitiveIndices.Add(PrimitiveBounds.Num() - 1);
    }

    // 2. 노드 배열 예약 (최악의 경우 2*N-1개 노드)
    Nodes.Reserve(PrimitiveBounds.Num() * 2);

    // 3. 재귀적으로 BVH 구축
    MaxDepth = 0;
    int RootIndex = BuildRecursive(0, PrimitiveBounds.Num(), 0);

    uint64_t BuildCycles = BVHBuildTimer.Finish();
    double BuildTimeMs = FPlatformTime::ToMilliseconds(BuildCycles);

    char buf[256];
    /*sprintf_s(buf, "[BVH] Built for %d actors, %d nodes, depth %d (Time: %.3fms)\n",
        PrimitiveBounds.Num(), Nodes.Num(), MaxDepth, BuildTimeMs);
    UE_LOG(buf);*/
}


void FBVH::Clear()
{
    Nodes.Empty();
    PrimitiveBounds.Empty();
    PrimitiveIndices.Empty();
    MaxDepth = 0;
}

TArray<FVector> FBVH::GetBVHBoundsWire() const
{
    TArray<FVector> BoundsWire;
    for (const FBVHNode& Node : Nodes)
    {
        BoundsWire.Append(Node.BoundingBox.GetWireLine());
    }
    return BoundsWire;
}
TArray<UPrimitiveComponent*> FBVH::GetCollisionWithOBB(const FOBB& OBB) const
{
    TArray<UPrimitiveComponent*> Primitives;

    if (Nodes.Num() > 0)
    {
        GetCollisionWithOBBRecursive(OBB, 0, Primitives);
    }
    return Primitives;
}
void FBVH::GetCollisionWithOBBRecursive(const FOBB& OBB, int NodeIdx, TArray<UPrimitiveComponent*>& HitPrimitives) const
{
    const FBVHNode& Node = Nodes[NodeIdx];

    //Node AABB, OBB 충돌 체크
    if (IntersectOBBAABB(OBB, Node.BoundingBox)) 
    {
        if (Node.IsLeaf())
        {
            for (int i = 0; i < Node.PrimitiveCount; i++)
            {
                int CurIdx = PrimitiveIndices[Node.FirstPrimitive + i];
                if (IntersectOBBAABB(OBB, PrimitiveBounds[CurIdx].AABB))
                {
                    HitPrimitives.Push(PrimitiveBounds[CurIdx].Primitive);
                }
            }
        }
        else
        {
            GetCollisionWithOBBRecursive(OBB, Node.LeftChild, HitPrimitives);
            GetCollisionWithOBBRecursive(OBB, Node.RightChild, HitPrimitives);
        }
    }
}

UPrimitiveComponent* FBVH::Intersect(const FVector& RayOrigin, const FVector& RayDirection, float& OutDistance) const
{
    if (Nodes.Num() == 0)
        return nullptr;

    TStatId BVHIntersectStatId;
    FScopeCycleCounter BVHIntersectTimer(BVHIntersectStatId);

    OutDistance = FLT_MAX;
    UPrimitiveComponent* HitPrimitive = nullptr;

    // 최적화된 Ray 생성 (InverseDirection과 Sign 미리 계산)
    FOptimizedRay OptRay(RayOrigin, RayDirection);
    bool bHit = IntersectNode(0, OptRay, OutDistance, HitPrimitive);

    uint64_t IntersectCycles = BVHIntersectTimer.Finish();
    double IntersectTimeMs = FPlatformTime::ToMilliseconds(IntersectCycles);

    if (bHit)
    {
        char buf[256];
        sprintf_s(buf, "[BVH Pick] Hit actor at distance %.3f (Time: %.3fms)\n",
            OutDistance, IntersectTimeMs);
        UE_LOG(buf);
        return HitPrimitive;
    }
    else
    {
        char buf[256];
        sprintf_s(buf, "[BVH Pick] No hit (Time: %.3fms)\n", IntersectTimeMs);
        UE_LOG(buf);
        return nullptr;
    }
}

int FBVH::BuildRecursive(int FirstPrimitive, int PrimitiveCount, int Depth)
{
    MaxDepth = FMath::Max(MaxDepth, Depth);

    int NodeIndex = Nodes.Num();
    FBVHNode NewNode;
    Nodes.Add(NewNode);
    FBVHNode& Node = Nodes[NodeIndex];

    Node.BoundingBox = CalculateBounds(FirstPrimitive, PrimitiveCount);

    if (PrimitiveCount <= MaxActorsPerLeaf || Depth >= MaxBVHDepth)
    {
        Node.FirstPrimitive = FirstPrimitive;
        Node.PrimitiveCount = PrimitiveCount;
        return NodeIndex;
    }

    int BestAxis;
    float BestSplitPos;
    int SplitIndex = FindBestSplit(FirstPrimitive, PrimitiveCount, BestAxis, BestSplitPos);

    if (SplitIndex == FirstPrimitive || SplitIndex == FirstPrimitive + PrimitiveCount)
    {
        Node.FirstPrimitive = FirstPrimitive;
        Node.PrimitiveCount = PrimitiveCount;
        return NodeIndex;
    }

    int ActualSplit = PartitionActors(FirstPrimitive, PrimitiveCount, BestAxis, BestSplitPos);

    int LeftCount = ActualSplit - FirstPrimitive;
    int RightCount = PrimitiveCount - LeftCount;

    if (LeftCount == 0 || RightCount == 0)
    {
        Node.FirstPrimitive = FirstPrimitive;
        Node.PrimitiveCount = PrimitiveCount;
        return NodeIndex;
    }

    Node.LeftChild = BuildRecursive(FirstPrimitive, LeftCount, Depth + 1);
    Node.RightChild = BuildRecursive(ActualSplit, RightCount, Depth + 1);

    return NodeIndex;
}


FAABB FBVH::CalculateBounds(int FirstPrimitive, int PrimitiveCount) const
{
    FAABB Bounds;
    bool bFirst = true;

    for (int i = 0; i < PrimitiveCount; ++i)
    {
        int ActorIndex = PrimitiveIndices[FirstPrimitive + i];
        const FAABB& ActorBound = PrimitiveBounds[ActorIndex].AABB;

        if (bFirst)
        {
            Bounds = ActorBound;
            bFirst = false;
        }
        else
        {
            Bounds.Min.X = FMath::Min(Bounds.Min.X, ActorBound.Min.X);
            Bounds.Min.Y = FMath::Min(Bounds.Min.Y, ActorBound.Min.Y);
            Bounds.Min.Z = FMath::Min(Bounds.Min.Z, ActorBound.Min.Z);

            Bounds.Max.X = FMath::Max(Bounds.Max.X, ActorBound.Max.X);
            Bounds.Max.Y = FMath::Max(Bounds.Max.Y, ActorBound.Max.Y);
            Bounds.Max.Z = FMath::Max(Bounds.Max.Z, ActorBound.Max.Z);
        }
    }

    return Bounds;
}

FAABB FBVH::CalculateCentroidBounds(int FirstPrimitive, int PrimitiveCount) const
{
    FAABB Bounds;
    bool bFirst = true;

    for (int i = 0; i < PrimitiveCount; ++i)
    {
        int ActorIndex = PrimitiveIndices[FirstPrimitive + i];
        const FVector& Center = PrimitiveBounds[ActorIndex].Center;

        if (bFirst)
        {
            Bounds.Min = Bounds.Max = Center;
            bFirst = false;
        }
        else
        {
            Bounds.Min.X = FMath::Min(Bounds.Min.X, Center.X);
            Bounds.Min.Y = FMath::Min(Bounds.Min.Y, Center.Y);
            Bounds.Min.Z = FMath::Min(Bounds.Min.Z, Center.Z);

            Bounds.Max.X = FMath::Max(Bounds.Max.X, Center.X);
            Bounds.Max.Y = FMath::Max(Bounds.Max.Y, Center.Y);
            Bounds.Max.Z = FMath::Max(Bounds.Max.Z, Center.Z);
        }
    }

    return Bounds;
}

int FBVH::FindBestSplit(int FirstPrimitive, int PrimitiveCount, int& OutAxis, float& OutSplitPos)
{
    FAABB CentroidBounds = CalculateCentroidBounds(FirstPrimitive, PrimitiveCount);
    FAABB ParentBounds = CalculateBounds(FirstPrimitive, PrimitiveCount);

    FVector Extent = CentroidBounds.Max - CentroidBounds.Min;
    OutAxis = 0;
    if (Extent.Y > Extent.X) OutAxis = 1;
    if (Extent.Z > Extent[OutAxis]) OutAxis = 2;

    if (Extent[OutAxis] < KINDA_SMALL_NUMBER)
    {
        OutSplitPos = CentroidBounds.Min[OutAxis];
        return FirstPrimitive + PrimitiveCount / 2;
    }

    // 1) 정렬 - TArray의 Sort 사용
    // 임시 배열 생성 후 정렬
    TArray<int> TempIndices;
    TempIndices.Reserve(PrimitiveCount);
    for (int i = 0; i < PrimitiveCount; ++i)
    {
        TempIndices.Add(PrimitiveIndices[FirstPrimitive + i]);
    }

    TempIndices.Sort([&](int A, int B)
    {
        return PrimitiveBounds[A].Center[OutAxis] < PrimitiveBounds[B].Center[OutAxis];
    });

    // 정렬된 결과를 다시 복사
    for (int i = 0; i < PrimitiveCount; ++i)
    {
        PrimitiveIndices[FirstPrimitive + i] = TempIndices[i];
    }
    // 2) Prefix/Suffix AABB 계산
    TArray<FAABB> Prefix;
    TArray<FAABB> Suffix;
    Prefix.SetNum(PrimitiveCount);
    Suffix.SetNum(PrimitiveCount);

    Prefix[0] = PrimitiveBounds[PrimitiveIndices[FirstPrimitive]].AABB;
    for (int i = 1; i < PrimitiveCount; i++)
    {
        Prefix[i] = Prefix[i - 1] + PrimitiveBounds[PrimitiveIndices[FirstPrimitive + i]].AABB;
    }

    Suffix[PrimitiveCount - 1] = PrimitiveBounds[PrimitiveIndices[FirstPrimitive + PrimitiveCount - 1]].AABB;
    for (int i = PrimitiveCount - 2; i >= 0; i--)
    {
        Suffix[i] = Suffix[i + 1] + PrimitiveBounds[PrimitiveIndices[FirstPrimitive + i]].AABB;
    }

    // 3) SAH 비용 평가
    float BestCost = FLT_MAX;
    int BestSplit = FirstPrimitive + PrimitiveCount / 2;
    float SA_P = ParentBounds.GetSurfaceArea() + 1e-6f;

    for (int i = 0; i < PrimitiveCount - 1; i++)
    {
        int LeftCount = i + 1;
        int RightCount = PrimitiveCount - LeftCount;

        float SA_L = Prefix[i].GetSurfaceArea();
        float SA_R = Suffix[i + 1].GetSurfaceArea();

        float Cost = 1.0f + (SA_L / SA_P) * LeftCount + (SA_R / SA_P) * RightCount;

        if (Cost < BestCost)
        {
            BestCost = Cost;
            BestSplit = FirstPrimitive + LeftCount;
            OutSplitPos = PrimitiveBounds[PrimitiveIndices[BestSplit]].Center[OutAxis];
        }
    }

    return BestSplit;
}

//static inline float SurfaceArea(const FAABB& b) {
//    FVector s = b.Max - b.Min;
//    if (s.X <= 0 || s.Y <= 0 || s.Z <= 0) return 0.0f;
//    return 2.0f * (s.X * s.Y + s.Y * s.Z + s.Z * s.X);
//}

float FBVH::CalculateSAH(int FirstPrimitive, int LeftCount, int RightCount, const FAABB& Parent) const
{
    FAABB LB = CalculateBounds(FirstPrimitive, LeftCount);
    FAABB RB = CalculateBounds(FirstPrimitive + LeftCount, RightCount);

    float SA_P = Parent.GetSurfaceArea() + 1e-6f;
    float SA_L = LB.GetSurfaceArea();
    float SA_R = RB.GetSurfaceArea();

    constexpr float Ct = 1.0f;
    constexpr float Ci = 1.0f;
    return Ct + Ci * ((SA_L / SA_P) * LeftCount + (SA_R / SA_P) * RightCount);
}

int FBVH::PartitionActors(int FirstPrimitive, int PrimitiveCount, int Axis, float SplitPos)
{
    int Left = FirstPrimitive;
    int Right = FirstPrimitive + PrimitiveCount - 1;

    while (Left <= Right)
    {
        while (Left <= Right)
        {
            int LeftActorIndex = PrimitiveIndices[Left];
            const FVector& LeftCenter = PrimitiveBounds[LeftActorIndex].Center;
            if (LeftCenter[Axis] >= SplitPos)
                break;
            Left++;
        }

        while (Left <= Right)
        {
            int RightActorIndex = PrimitiveIndices[Right];
            const FVector& RightCenter = PrimitiveBounds[RightActorIndex].Center;
            if (RightCenter[Axis] < SplitPos)
                break;
            Right--;
        }

        if (Left < Right)
        {
            int Temp = PrimitiveIndices[Left];
            PrimitiveIndices[Left] = PrimitiveIndices[Right];
            PrimitiveIndices[Right] = Temp;
            Left++;
            Right--;
        }
    }

    return Left;
}

bool FBVH::IntersectNode(int NodeIndex,
    const FOptimizedRay& Ray,
    float& InOutDistance,
    UPrimitiveComponent*& OutPrimitive) const
{
    const FBVHNode& Node = Nodes[NodeIndex];

    // 최적화된 Ray-AABB 교차 검사 사용
    float tNear;
    if (!IntersectOptRayAABB(Ray, Node.BoundingBox, tNear))
        return false;

    if (tNear >= InOutDistance)
        return false;

    if (Node.IsLeaf())
    {
        bool bHit = false;
        float Closest = InOutDistance;
        UPrimitiveComponent* ClosetPrimitive = nullptr;

        for (int i = 0; i < Node.PrimitiveCount; ++i)
        {
            int ActorIndex = PrimitiveIndices[Node.FirstPrimitive + i];
            UPrimitiveComponent* Primitive = PrimitiveBounds[ActorIndex].Primitive;

            float Dist;
            if (IntersectRayAABB(Ray, Primitive->GetWorldAABB(), Dist))
            {
                if (Dist < Closest)
                {
                    Closest = Dist;
                    ClosetPrimitive = Primitive;
                    bHit = true;
                }
            }
        }

        if (bHit)
        {
            InOutDistance = Closest;
            OutPrimitive = ClosetPrimitive;
        }

        return bHit;
    }

    struct ChildHit
    {
        int Index;
        float tNear;
        bool bValid;
    };

    auto TestChild = [&](int ChildIdx) -> ChildHit
        {
            if (ChildIdx < 0) return { ChildIdx, FLT_MAX, false };
            float tN;
            if (IntersectOptRayAABB(Ray,Nodes[ChildIdx].BoundingBox,tN))
                return { ChildIdx, tN, true };
            return { ChildIdx, FLT_MAX, false };
        };

    ChildHit L = TestChild(Node.LeftChild);
    ChildHit R = TestChild(Node.RightChild);

    bool bHit = false;

    if (L.bValid && R.bValid)
    {
        const ChildHit First = (L.tNear < R.tNear) ? L : R;
        const ChildHit Second = (L.tNear < R.tNear) ? R : L;

        if (IntersectNode(First.Index, Ray, InOutDistance, OutPrimitive))
            bHit = true;

        if (InOutDistance > Second.tNear)
        {
            if (IntersectNode(Second.Index, Ray, InOutDistance, OutPrimitive))
                bHit = true;
        }
    }
    else if (L.bValid)
    {
        if (IntersectNode(L.Index, Ray, InOutDistance, OutPrimitive))
            bHit = true;
    }
    else if (R.bValid)
    {
        if (IntersectNode(R.Index, Ray, InOutDistance, OutPrimitive))
            bHit = true;
    }

    return bHit;
}