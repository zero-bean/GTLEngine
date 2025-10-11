#pragma once

#include "Global/CoreTypes.h"
#include "Global/Vector.h"
#include "Physics/Public/AABB.h"
#include "Physics/Public/RayIntersection.h"
#include "Component/Mesh/Public/VertexDatas.h"

// Per-triangle BVH primitive used by StaticMesh triangle acceleration
struct FTriangleBVHPrimitive
{
    FAABB Bounds;
    FVector Center;
    uint32 Indices[3];

    // Precomputed for fast intersection in model space
    FVector V0;
    FVector E0; // V1 - V0
    FVector E1; // V2 - V0
};

struct FTriangleBVHNode
{
    FAABB Bounds;
    int32 LeftChild = -1;
    int32 RightChild = -1;
    int32 Start = 0;
    int32 Count = 0;
    bool bIsLeaf = false;
};

// Helper for building and raycasting a triangle BVH
class FTriangleBVH
{
public:
    // Build triangle primitives and BVH nodes. If Indices is empty, uses triplets of Vertices.
    static void Build(
        const TArray<FNormalVertex>& Vertices,
        const TArray<uint32>& Indices,
        TArray<FTriangleBVHPrimitive>& OutPrimitives,
        TArray<FTriangleBVHNode>& OutNodes,
        int32& OutRoot);

    // Raycast against a built triangle BVH in model space. Updates InOutDistance to closest hit if any.
    static bool Raycast(
        const FRay& ModelRay,
        float& InOutDistance,
        const TArray<FTriangleBVHPrimitive>& Primitives,
        const TArray<FTriangleBVHNode>& Nodes,
        int32 Root);
};

