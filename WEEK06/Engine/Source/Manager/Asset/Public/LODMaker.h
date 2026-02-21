#pragma once
#include "ObjImporter.h"

struct FMeshIndex
{
    int VertexIndex = -1;
    int NormalIndex = -1;
    int TexCoordIndex = -1;
};

struct FTriangle
{
    FMeshIndex Vertices[3];
    bool bIsActive = true;
};

struct FSimplificationVertex
{
    FVector Position;
    FMatrix Q;
    bool bIsActive = true;
};

struct FEdgeCollapsePair
{
    uint32 V1Index;
    uint32 V2Index;
    float Cost;
    FVector OptimalPosition;

    bool operator>(const FEdgeCollapsePair& Other) const
    {
        return Cost > Other.Cost;
    }
};

using TPriorityQueue = std::priority_queue<FEdgeCollapsePair, TArray<FEdgeCollapsePair>, std::greater<>>;

class FMeshSimplifier
{
public:
    bool LoadFromObj(const FString& InFilename);
    void Simplify(float InReductionRatio);
    bool SaveToObj(const FString& InFilename, float InReductionRatio);

private:
    void CalculateInitialQuadrics();
    TPriorityQueue CalculateAllEdgeCosts();
    FEdgeCollapsePair CalculateEdgeCost(uint32 InV1Index, uint32 InV2Index);
    uint32 CollapseEdge(uint32 InUIndex, uint32 InVIndex, const FVector& InOptimalPosition);

    TArray<FSimplificationVertex> SimplificationVertices;
    TArray<FTriangle> Triangles;
    FObjInfo ObjInfo;
};



