#include "pch.h"
#include "Core/Public/TriangleBVH.h"

#include <algorithm>
#include <functional>

// Internal helpers
namespace
{
    inline float SurfaceArea(const FAABB& b)
    {
        const FVector d = b.Max - b.Min;
        if (d.X <= 0.f || d.Y <= 0.f || d.Z <= 0.f) return 0.f;
        return 2.0f * (d.X*d.Y + d.Y*d.Z + d.Z*d.X);
    }

    inline void ExpandToFit(FAABB& dst, const FAABB& src)
    {
        dst.Min.X = std::min(dst.Min.X, src.Min.X);
        dst.Min.Y = std::min(dst.Min.Y, src.Min.Y);
        dst.Min.Z = std::min(dst.Min.Z, src.Min.Z);
        dst.Max.X = std::max(dst.Max.X, src.Max.X);
        dst.Max.Y = std::max(dst.Max.Y, src.Max.Y);
        dst.Max.Z = std::max(dst.Max.Z, src.Max.Z);
    }

    inline void ExpandToFitPoint(FAABB& dst, const FVector& p)
    {
        dst.Min.X = std::min(dst.Min.X, p.X);
        dst.Min.Y = std::min(dst.Min.Y, p.Y);
        dst.Min.Z = std::min(dst.Min.Z, p.Z);
        dst.Max.X = std::max(dst.Max.X, p.X);
        dst.Max.Y = std::max(dst.Max.Y, p.Y);
        dst.Max.Z = std::max(dst.Max.Z, p.Z);
    }

    // Safe division
    inline float SafeRcp(float x) { return (fabsf(x) > 1e-20f) ? (1.0f / x) : 0.0f; }

    template<int NBINS>
    struct FBin
    {
        FAABB b;
        int n;
        FBin()
            : b(FAABB(FVector(FLT_MAX, FLT_MAX, FLT_MAX), FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX)))
            , n(0) {}
    };
}

void FTriangleBVH::Build(
    const TArray<FNormalVertex>& Vertices,
    const TArray<uint32>& Indices,
    TArray<FTriangleBVHPrimitive>& OutPrimitives,
    TArray<FTriangleBVHNode>& OutNodes,
    int32& OutRoot)
{
    OutPrimitives.clear();
    OutNodes.clear();
    OutRoot = -1;

    const bool bHasIndices = !Indices.empty();
    const size_t TriangleCount = bHasIndices ? (Indices.size() / 3) : (Vertices.size() / 3);
    if (TriangleCount == 0)
    {
        return;
    }

    OutPrimitives.reserve(TriangleCount);

    auto AddPrimitive = [&](uint32 i0, uint32 i1, uint32 i2)
    {
        const FVector& P0 = Vertices[i0].Position;
        const FVector& P1 = Vertices[i1].Position;
        const FVector& P2 = Vertices[i2].Position;

        FVector minP(std::min({ P0.X, P1.X, P2.X }),
            std::min({ P0.Y, P1.Y, P2.Y }),
            std::min({ P0.Z, P1.Z, P2.Z }));
        FVector maxP(std::max({ P0.X, P1.X, P2.X }),
            std::max({ P0.Y, P1.Y, P2.Y }),
            std::max({ P0.Z, P1.Z, P2.Z }));

        FTriangleBVHPrimitive prim;
        prim.Bounds = FAABB(minP, maxP);
        prim.Center = (P0 + P1 + P2) / 3.0f;

        prim.Indices[0] = i0;
        prim.Indices[1] = i1;
        prim.Indices[2] = i2;

        prim.V0 = P0;
        prim.E0 = P1 - P0;
        prim.E1 = P2 - P0;

        OutPrimitives.push_back(prim);
    };

    if (bHasIndices)
    {
        for (size_t i = 0; i + 2 < Indices.size(); i += 3)
        {
            AddPrimitive(Indices[i + 0], Indices[i + 1], Indices[i + 2]);
        }
    }
    else
    {
        for (size_t i = 0; i + 2 < Vertices.size(); i += 3)
        {
            AddPrimitive(static_cast<uint32>(i + 0), static_cast<uint32>(i + 1), static_cast<uint32>(i + 2));
        }
    }

    if (OutPrimitives.empty())
    {
        return;
    }

    constexpr int32 MaxLeafSize = 8;
    static constexpr int NBINS = 16;
    static constexpr float Ct = 1.0f; // traversal cost
    static constexpr float Ci = 1.0f; // primitive cost

    std::function<int32(int32, int32)> BuildBVH;
    BuildBVH = [&](int32 Start, int32 Count) -> int32
    {
        // 1) Compute node bounds and centroid bounds
        FAABB nodeBounds(FVector(FLT_MAX, FLT_MAX, FLT_MAX), FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX));
        FAABB centroidBounds(FVector(FLT_MAX, FLT_MAX, FLT_MAX), FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX));

        for (int32 i = 0; i < Count; ++i)
        {
            const FTriangleBVHPrimitive& p = OutPrimitives[Start + i];
            ExpandToFit(nodeBounds, p.Bounds);
            ExpandToFitPoint(centroidBounds, p.Center);
        }

        // 2) Leaf condition
        if (Count <= MaxLeafSize)
        {
            FTriangleBVHNode leaf;
            leaf.Bounds = nodeBounds;
            leaf.bIsLeaf = true;
            leaf.Start = Start;
            leaf.Count = Count;
            const int32 idx = (int32)OutNodes.size();
            OutNodes.push_back(leaf);
            return idx;
        }

        // 3) Choose split axis by longest extent of centroids
        const FVector cExt = centroidBounds.Max - centroidBounds.Min;
        int axis = 0;
        if (cExt.Y > cExt.X) axis = 1;
        if (cExt.Z > (axis == 0 ? cExt.X : cExt.Y)) axis = 2;

        // 4) If degenerate centroid bounds, make a leaf to avoid looping
        if (cExt[axis] <= 0.0f)
        {
            FTriangleBVHNode leaf;
            leaf.Bounds = nodeBounds;
            leaf.bIsLeaf = true;
            leaf.Start = Start;
            leaf.Count = Count;
            const int32 idx = (int32)OutNodes.size();
            OutNodes.push_back(leaf);
            return idx;
        }

        // 5) SAH bins
        FBin<NBINS> bins[NBINS];

        const float rcpExtent = SafeRcp(cExt[axis]);
        auto binIndexOf = [&](const FVector& c)
        {
            const float t = (c[axis] - centroidBounds.Min[axis]) * rcpExtent;
            int b = (int)floorf(t * NBINS);
            if (b < 0) b = 0;
            if (b >= NBINS) b = NBINS - 1;
            return b;
        };

        for (int32 i = 0; i < Count; ++i)
        {
            const FTriangleBVHPrimitive& p = OutPrimitives[Start + i];
            const int bi = binIndexOf(p.Center);
            ExpandToFit(bins[bi].b, p.Bounds);
            bins[bi].n++;
        }

        // Prefix/suffix bounds
        FAABB leftBounds[NBINS - 1];
        FAABB rightBounds[NBINS - 1];
        int leftCount[NBINS - 1];
        int rightCount[NBINS - 1];

        FAABB accL( FVector(FLT_MAX,FLT_MAX,FLT_MAX), FVector(-FLT_MAX,-FLT_MAX,-FLT_MAX) );
        int cntL = 0;
        for (int i = 0; i < NBINS - 1; ++i)
        {
            ExpandToFit(accL, bins[i].b);
            cntL += bins[i].n;
            leftBounds[i] = accL;
            leftCount[i] = cntL;
        }

        FAABB accR( FVector(FLT_MAX,FLT_MAX,FLT_MAX), FVector(-FLT_MAX,-FLT_MAX,-FLT_MAX) );
        int cntR = 0;
        for (int i = NBINS - 1; i > 0; --i)
        {
            ExpandToFit(accR, bins[i].b);
            cntR += bins[i].n;
            if (i - 1 < NBINS - 1)
            {
                rightBounds[i - 1] = accR;
                rightCount[i - 1] = cntR;
            }
        }

        // 6) Find best split by SAH
        float bestCost = FLT_MAX;
        int bestSplitBin = -1;
        const float nodeSA = SurfaceArea(nodeBounds);

        for (int s = 0; s < NBINS - 1; ++s)
        {
            if (leftCount[s] == 0 || rightCount[s] == 0) continue;
            const float pL = SurfaceArea(leftBounds[s]) / nodeSA;
            const float pR = SurfaceArea(rightBounds[s]) / nodeSA;
            const float cost = Ct + Ci * (pL * leftCount[s] + pR * rightCount[s]);
            if (cost < bestCost)
            {
                bestCost = cost;
                bestSplitBin = s;
            }
        }

        // If SAH fails to find a valid split, make a leaf
        if (bestSplitBin < 0)
        {
            FTriangleBVHNode leaf;
            leaf.Bounds = nodeBounds;
            leaf.bIsLeaf = true;
            leaf.Start = Start;
            leaf.Count = Count;
            const int32 nodeIdx = (int32)OutNodes.size();
            OutNodes.push_back(leaf);
            return nodeIdx;
        }

        // 7) Partition primitives in-place by the chosen split
        auto beginIt = OutPrimitives.begin() + Start;
        auto endIt = beginIt + Count;
        const int splitBin = bestSplitBin;
        auto midIt = std::stable_partition(beginIt, endIt, [&](const FTriangleBVHPrimitive& p)
        {
            const int bi = binIndexOf(p.Center);
            return bi <= splitBin;
        });

        int32 leftCountFinal = int32(midIt - beginIt);
        int32 rightCountFinal = Count - leftCountFinal;

        // Guard: if one side is empty, fallback to median split
        if (leftCountFinal == 0 || rightCountFinal == 0)
        {
            const int32 mid = Start + Count / 2;
            auto key = [axis](const FTriangleBVHPrimitive& A, const FTriangleBVHPrimitive& B)
            {
                const float a = (axis == 0 ? A.Center.X : (axis == 1 ? A.Center.Y : A.Center.Z));
                const float b = (axis == 0 ? B.Center.X : (axis == 1 ? B.Center.Y : B.Center.Z));
                return a < b;
            };
            std::nth_element(beginIt, beginIt + (mid - Start), endIt, key);
            leftCountFinal = mid - Start;
            rightCountFinal = Count - leftCountFinal;
            midIt = beginIt + leftCountFinal;
        }

        // 8) Create inner node and recurse
        const int32 leftChild = BuildBVH(Start, leftCountFinal);
        const int32 rightChild = BuildBVH(Start + leftCountFinal, rightCountFinal);

        FTriangleBVHNode node;
        node.Bounds = nodeBounds;
        node.bIsLeaf = false;
        node.Start = Start;
        node.Count = Count;
        node.LeftChild = leftChild;
        node.RightChild = rightChild;

        const int32 nodeIdx = (int32)OutNodes.size();
        OutNodes.push_back(node);
        return nodeIdx;
    };

    OutRoot = BuildBVH(0, static_cast<int32>(OutPrimitives.size()));
}

bool FTriangleBVH::Raycast(
    const FRay& ModelRay,
    float& InOutDistance,
    const TArray<FTriangleBVHPrimitive>& Primitives,
    const TArray<FTriangleBVHNode>& Nodes,
    int32 Root)
{
    if (Root < 0 || Nodes.empty()) return false;

    static thread_local int32 Stack[128];
    int32 sp = 0;
    Stack[sp++] = Root;

    bool hit = false;
    float closest = InOutDistance;

    while (sp > 0)
    {
        const int32 ni = Stack[--sp];
        const FTriangleBVHNode& node = Nodes[ni];

        float tEntry;
        if (!node.Bounds.RaycastHit(ModelRay, &tEntry) || tEntry > closest)
            continue;

        if (node.bIsLeaf)
        {
            for (int32 i = 0; i < node.Count; ++i)
            {
                const FTriangleBVHPrimitive& p = Primitives[node.Start + i];

                float tTriBoxEntry;
                if (!p.Bounds.RaycastHit(ModelRay, &tTriBoxEntry) || tTriBoxEntry > closest)
                    continue;

                float t;
                if (RayTriangleIntersectModelPrecomputed(ModelRay, p.V0, p.E0, p.E1, closest, t))
                {
                    closest = t;
                    hit = true;
                }
            }
        }
        else
        {
            float tL = FLT_MAX, tR = FLT_MAX;
            const bool hasL = (node.LeftChild != -1) && Nodes[node.LeftChild].Bounds.RaycastHit(ModelRay, &tL) && tL <= closest;
            const bool hasR = (node.RightChild != -1) && Nodes[node.RightChild].Bounds.RaycastHit(ModelRay, &tR) && tR <= closest;

            if (hasL && hasR)
            {
                if (tL < tR)
                {
                    if (sp < 128) Stack[sp++] = node.RightChild;
                    if (sp < 128) Stack[sp++] = node.LeftChild;
                }
                else
                {
                    if (sp < 128) Stack[sp++] = node.LeftChild;
                    if (sp < 128) Stack[sp++] = node.RightChild;
                }
            }
            else if (hasL)
            {
                if (sp < 128) Stack[sp++] = node.LeftChild;
            }
            else if (hasR)
            {
                if (sp < 128) Stack[sp++] = node.RightChild;
            }
        }
    }

    if (hit) InOutDistance = closest;
    return hit;
}

