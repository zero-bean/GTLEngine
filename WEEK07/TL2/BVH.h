#pragma once
#include "Vector.h"
#include "UEContainer.h"
#include "BoundingVolume.h"
#include "Actor.h"
#include <cmath>



// BVH 노드 구조체
struct FBVHNode
{
    FAABB BoundingBox;

    // 리프 노드용 데이터
    int FirstPrimitive;  // 리프인 경우: 첫 번째 액터 인덱스, 내부 노드인 경우: -1
    int PrimitiveCount;  // 리프인 경우: 액터 개수, 내부 노드인 경우: -1

    // 내부 노드용 데이터
    int LeftChild;   // 왼쪽 자식 노드 인덱스
    int RightChild;  // 오른쪽 자식 노드 인덱스

    // 생성자
    FBVHNode()
        : FirstPrimitive(-1), PrimitiveCount(-1), LeftChild(-1), RightChild(-1)
    {
    }

    // 리프 노드인지 확인
    bool IsLeaf() const { return FirstPrimitive >= 0 && PrimitiveCount > 0; }
};

// 액터의 AABB와 포인터를 저장
struct FBVHPtimitive
{
    FAABB AABB;
    UPrimitiveComponent* Primitive;
    FVector Center;

    FBVHPtimitive() : Primitive(nullptr) {}
    FBVHPtimitive(UPrimitiveComponent* InPrimitive, const FAABB& InBounds)
        : Primitive(InPrimitive), AABB(InBounds)
    {
        Center = InBounds.GetCenter();
    }
};

// 고성능 BVH 구현
class FBVH
{
public:
    FBVH();
    ~FBVH();

    // 액터 배열로부터 BVH 구축
    void Build(const TArray<AActor*>& Actors);
    void Build(const TArray<UPrimitiveComponent*>& Primitives);
    void Clear();

    // 빠른 레이 교차 검사 - 가장 가까운 액터 반환
    UPrimitiveComponent* Intersect(const FVector& RayOrigin, const FVector& RayDirection, float& OutDistance) const;

    // 통계 정보
    int GetNodeCount() const { return Nodes.Num(); }
    int GetMeshCount() const { return PrimitiveBounds.Num(); }
    int GetMaxDepth() const { return MaxDepth; }

    // 렌더링을 위한 노드 접근
    const TArray<FBVHNode>& GetNodes() const { return Nodes; }

    TArray<FVector> GetBVHBoundsWire() const;

    TArray<UPrimitiveComponent*> GetCollisionWithOBB(const FOBB& OBB) const;

    bool IsBuild() { return Nodes.Num() > 0; }
private:
    TArray<FBVHNode> Nodes;
    TArray<FBVHPtimitive> PrimitiveBounds;
    TArray<int> PrimitiveIndices; // 정렬된 액터 인덱스

    int MaxDepth;

    // 재귀 구축 함수
    int BuildRecursive(int FirstActor, int ActorCount, int Depth = 0);

    void GetCollisionWithOBBRecursive(const FOBB& OBB, int NodeIdx, TArray<UPrimitiveComponent*>& HitPrimitives) const;
    // 경계 박스 계산
    FAABB CalculateBounds(int FirstActor, int ActorCount) const;
    FAABB CalculateCentroidBounds(int FirstActor, int ActorCount) const;

    // Surface Area Heuristic을 이용한 최적 분할
    int FindBestSplit(int FirstActor, int ActorCount, int& OutAxis, float& OutSplitPos);
    float CalculateSAH(int FirstActor, int LeftCount, int RightCount, const FAABB& ParentBounds) const;

    // 액터 분할
    int PartitionActors(int FirstActor, int ActorCount, int Axis, float SplitPos);

    bool IntersectNode(int NodeIndex, const FOptimizedRay& Ray, float& InOutDistance, UPrimitiveComponent*& OutPrimitive) const;

    //// 재귀 교차 검사 (깊이 제한 추가)
    //bool IntersectNode(int NodeIndex, const FVector& RayOrigin, const FVector& RayDirection,
    //                   float& InOutDistance, AActor*& OutActor, int RecursionDepth = 0) const;

    // 상수
    static const int MaxActorsPerLeaf = 8;  // 리프당 최대 액터 수 (마이크로 BVH용)
    static const int MaxBVHDepth = 24;      // 최대 깊이
    static const int SAHSamples = 10;       // SAH 샘플링 수
};