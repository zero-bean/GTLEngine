#pragma once
#include "Vector.h"
#include "Enums.h"

struct FOptimizedRay;

// Unreal-style simple ray type
// 이거 어디둘지 모르겠음
struct alignas(16) FRay
{
    FVector Origin;
    FVector Direction; // Normalized
    FRay() = default;
    FRay(const FOptimizedRay& OptRay);
};

// 최적화된 Ray-AABB 교차 검사를 위한 구조체
struct alignas(16) FOptimizedRay
{
    FVector Origin;
    FVector Direction;
    FVector InverseDirection;  // 1.0f / Direction (division 제거용)
    int Sign[3];              // Direction의 부호 (branchless용)
    FOptimizedRay() = default;
    FOptimizedRay(const FVector& InOrigin, const FVector& InDirection)
        : Origin(InOrigin), Direction(InDirection)
    {
        // InverseDirection 계산 (0으로 나누기 방지)
        InverseDirection.X = (std::abs(InDirection.X) < KINDA_SMALL_NUMBER) ?
            (InDirection.X < 0.0f ? -1e30f : 1e30f) : 1.0f / InDirection.X;
        InverseDirection.Y = (std::abs(InDirection.Y) < KINDA_SMALL_NUMBER) ?
            (InDirection.Y < 0.0f ? -1e30f : 1e30f) : 1.0f / InDirection.Y;
        InverseDirection.Z = (std::abs(InDirection.Z) < KINDA_SMALL_NUMBER) ?
            (InDirection.Z < 0.0f ? -1e30f : 1e30f) : 1.0f / InDirection.Z;

        // Sign 계산 (branchless slab method용)
        Sign[0] = (InverseDirection.X < 0.0f) ? 1 : 0;
        Sign[1] = (InverseDirection.Y < 0.0f) ? 1 : 0;
        Sign[2] = (InverseDirection.Z < 0.0f) ? 1 : 0;
    }
};

struct FAABB
{
    FVector Min = FVector(0, 0, 0);
    FVector Max = FVector(0, 0, 0);
    FAABB() = default;
    FAABB(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax) {}
    FAABB(const FVector* Vertices, const uint32 Count)
    {
        InitAABB(Vertices, Count);
    }
    FAABB(const TArray<FNormalVertex>& NormalVertices)
    {
        InitAABB(NormalVertices);
    }
    FAABB(const TArray<FVector>& Vertices)
    {
        InitAABB(Vertices);
    }
    void InitAABB(const TArray<FNormalVertex>& NormalVertices)
    {
        if (NormalVertices.Num() <= 0)
        {
            return;
        }
        Min = NormalVertices[0].Pos;
        Max = NormalVertices[0].Pos;
        for (const FNormalVertex& Vertex : NormalVertices)
        {
            Min.X = std::min(Min.X, Vertex.Pos.X);
            Min.Y = std::min(Min.Y, Vertex.Pos.Y);
            Min.Z = std::min(Min.Z, Vertex.Pos.Z);

            Max.X = std::max(Max.X, Vertex.Pos.X);
            Max.Y = std::max(Max.Y, Vertex.Pos.Y);
            Max.Z = std::max(Max.Z, Vertex.Pos.Z);
        }
    }

    void InitAABB(const TArray<FVector>& Vertices)
    {
        if (Vertices.Num() <= 0)
        {
            return;
        }
        Min = Vertices[0];
        Max = Vertices[0];
        for (const FVector& Vertex : Vertices)
        {
            Min.X = std::min(Min.X, Vertex.X);
            Min.Y = std::min(Min.Y, Vertex.Y);
            Min.Z = std::min(Min.Z, Vertex.Z);

            Max.X = std::max(Max.X, Vertex.X);
            Max.Y = std::max(Max.Y, Vertex.Y);
            Max.Z = std::max(Max.Z, Vertex.Z);
        }
    }

    void InitAABB(const FVector* Vertices, const uint32 Count)
    {
        if (Count <= 0)
        {
            return;
        }
        Min = Vertices[0];
        Max = Vertices[0];
        for (int i = 0; i < Count; i++)
        {
            FVector Vertex = Vertices[i];
            Min.X = std::min(Min.X, Vertex.X);
            Min.Y = std::min(Min.Y, Vertex.Y);
            Min.Z = std::min(Min.Z, Vertex.Z);

            Max.X = std::max(Max.X, Vertex.X);
            Max.Y = std::max(Max.Y, Vertex.Y);
            Max.Z = std::max(Max.Z, Vertex.Z);
        }
    }
    FAABB& operator+=(const FAABB& Other)
    {
        Min.X = std::min(Min.X, Other.Min.X);
        Min.Y = std::min(Min.Y, Other.Min.Y);
        Min.Z = std::min(Min.Z, Other.Min.Z);

        Max.X = std::max(Max.X, Other.Max.X);
        Max.Y = std::max(Max.Y, Other.Max.Y);
        Max.Z = std::max(Max.Z, Other.Max.Z);

        return *this;
    }
    FAABB operator+(const FAABB& Other)
    {
        FAABB result;
        result.Min.X = std::min(Min.X, Other.Min.X);
        result.Min.Y = std::min(Min.Y, Other.Min.Y);
        result.Min.Z = std::min(Min.Z, Other.Min.Z);

        result.Max.X = std::max(Max.X, Other.Max.X);
        result.Max.Y = std::max(Max.Y, Other.Max.Y);
        result.Max.Z = std::max(Max.Z, Other.Max.Z);

        return *this;
    }
    FVector GetVertex(uint32 idx) const
    {
        idx = idx % 8;
        FVector Vertex;
        Vertex.X = (idx & 1) == 0 ? Min.X : Max.X;
        Vertex.Y = (idx & (1 << 1)) == 0 ? Min.Y : Max.Y;
        Vertex.Z = (idx & (1 << 2)) == 0 ? Min.Z : Max.Z;
        return Vertex;
    }
    // 센터 반환
    FVector GetCenter() const
    {
        return (Min + Max) * 0.5f;
    }
    // 박스의 절반 크기
    FVector GetExtent() const
    {
        return (Max - Min) * 0.5f;
    }

    // 표면적 계산 (SAH용)
    float GetSurfaceArea() const
    {
        FVector size = Max - Min;
        if (size.X <= 0.0f || size.Y <= 0.0f || size.Z <= 0.0f) return 0.0f;
        return 2.0f * (size.X * size.Y + size.Y * size.Z + size.Z * size.X);
    }
    bool IsInside(const FVector& Point) const
    {
        return (Point.X >= Min.X && Point.X <= Max.X &&
            Point.Y >= Min.Y && Point.Y <= Max.Y &&
            Point.Z >= Min.Z && Point.Z <= Max.Z);
    }
    bool IsIntersect(const FAABB& Other) const
    {
        return (Min.X <= Other.Max.X && Max.X >= Other.Min.X &&
            Min.Y <= Other.Max.Y && Max.Y >= Other.Min.Y &&
            Min.Z <= Other.Max.Z && Max.Z >= Other.Min.Z);
    }
    TArray<FVector> GetWireLine() const
    {
        if (Min == Max)
        {
            return {};
        }

        return{
            GetVertex(0), GetVertex(1),
            GetVertex(1), GetVertex(3),
            GetVertex(3), GetVertex(2),
            GetVertex(2), GetVertex(0),
            GetVertex(4), GetVertex(5),
            GetVertex(5), GetVertex(7),
            GetVertex(7), GetVertex(6),
            GetVertex(6), GetVertex(4),
            GetVertex(0), GetVertex(4),
            GetVertex(1), GetVertex(5),
            GetVertex(2), GetVertex(6),
            GetVertex(3), GetVertex(7),
        };
    }
};

struct FOBB
{
    FVector Center;
    FVector Axis[3]; //Forward, Right, Up
    FVector Extents; //half size (x(Forward), y(Right), z(Up))
    FOBB() = default;

    //AABB + WorldMatrix 생성자
    FOBB(const FAABB& LocalAABB, const FTransform& WorldTransform)
    {
        LocalAABBWorldTransformCalc(LocalAABB, WorldTransform);
    }
    void LocalAABBWorldTransformCalc(const FAABB& LocalAABB, const FTransform& WorldTransform)
    {
        FVector LocalAABBCenter = LocalAABB.GetCenter();
        FVector LocalAABBExtent = LocalAABB.GetExtent();
        FVector LocalAABBAxis[3] = { FVector(1,0,0),FVector(0,1,0),FVector(0,0,1) };

        FMatrix WorldMatrix = WorldTransform.ToMatrixWithScaleLocalXYZ();
        FVector4 WorldCenter = FVector4(LocalAABBCenter, 1) * WorldMatrix;
        Extents = LocalAABBExtent * WorldTransform.Scale3D;
        Axis[0] = WorldTransform.Rotation.RotateVector(LocalAABBAxis[0]);
        Axis[1] = WorldTransform.Rotation.RotateVector(LocalAABBAxis[1]);
        Axis[2] = WorldTransform.Rotation.RotateVector(LocalAABBAxis[2]);
        Center = FVector(WorldCenter.X, WorldCenter.Y, WorldCenter.Z);
    }

    FVector GetVertex(uint32 idx) const
    {
        idx = idx % 8;
        FVector Vertex = Center;
        Vertex += (idx & 1) == 0 ? Axis[0] * Extents.X : -Axis[0] * Extents.X;
        Vertex += (idx & (1 << 1)) == 0 ? Axis[1] * Extents.Y : -Axis[1] * Extents.Y;
        Vertex += (idx & (1 << 2)) == 0 ? Axis[2] * Extents.Z : -Axis[2] * Extents.Z;
        return Vertex;
    }

    TArray<FVector> GetWireLine() const
    {
        if (Extents == FVector(0,0,0))
        {
            return {};
        }

        return{
            GetVertex(0), GetVertex(1),
            GetVertex(1), GetVertex(3),
            GetVertex(3), GetVertex(2),
            GetVertex(2), GetVertex(0),
            GetVertex(4), GetVertex(5),
            GetVertex(5), GetVertex(7),
            GetVertex(7), GetVertex(6),
            GetVertex(6), GetVertex(4),
            GetVertex(0), GetVertex(4),
            GetVertex(1), GetVertex(5),
            GetVertex(2), GetVertex(6),
            GetVertex(3), GetVertex(7),
        };
    }
    
};


// 최적화된 Ray-AABB 교차 검사 (branchless slab method)
bool IntersectOptRayAABB(const FOptimizedRay& Ray, const FAABB& AABB, float& OutTNear);

bool IntersectRayAABB(const FRay& Ray, const FAABB& AABB, float& OutDistance);

bool IntersectOBBAABB(const FOBB& OBB, const FAABB& AABB);