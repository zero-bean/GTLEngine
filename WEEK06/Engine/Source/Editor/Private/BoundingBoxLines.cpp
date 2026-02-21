#include "pch.h"
#include "Editor/Public/BoundingBoxLines.h"
#include "Physics/Public/AABB.h"

UBoundingBoxLines::UBoundingBoxLines()
	: Vertices(TArray<FVector>())
	, NumVertices(8)
{
	Vertices.reserve(NumVertices);
	UpdateVertices(FAABB({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
}

void UBoundingBoxLines::MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex)
{
	// 인덱스 범위 보정
	if (insertStartIndex > destVertices.size())
		insertStartIndex = destVertices.size();

	// 미리 메모리 확보
	destVertices.reserve(destVertices.size() + std::distance(Vertices.begin(), Vertices.end()));

	// 덮어쓸 수 있는 개수 계산
	size_t overwriteCount = std::min(
		Vertices.size(),
		destVertices.size() - insertStartIndex
	);

	// 기존 요소 덮어쓰기
	std::copy(
		Vertices.begin(),
		Vertices.begin() + overwriteCount,
		destVertices.begin() + insertStartIndex
	);
}

void UBoundingBoxLines::UpdateVertices(FAABB boundingBoxInfo)
{
	// 중복 삽입 방지
	if (RenderedAABBoxInfo.Min == boundingBoxInfo.Min && RenderedAABBoxInfo.Max == boundingBoxInfo.Max)
	{
		return;
	}

	float MinX = boundingBoxInfo.Min.X, MinY = boundingBoxInfo.Min.Y, MinZ = boundingBoxInfo.Min.Z;
	float MaxX = boundingBoxInfo.Max.X, MaxY = boundingBoxInfo.Max.Y, MaxZ = boundingBoxInfo.Max.Z;

	if (Vertices.size() < NumVertices)
	{
		Vertices.resize(NumVertices);
	}

	// 꼭짓점 정의 (0~3: 앞면, 4~7: 뒷면)
	uint32 vertexIndex = 0;
	Vertices[vertexIndex++] = { MinX, MinY, MinZ }; // Front-Bottom-Left
	Vertices[vertexIndex++] = { MaxX, MinY, MinZ }; // Front-Bottom-Right
	Vertices[vertexIndex++] = { MaxX, MaxY, MinZ }; // Front-Top-Right
	Vertices[vertexIndex++] = { MinX, MaxY, MinZ }; // Front-Top-Left
	Vertices[vertexIndex++] = { MinX, MinY, MaxZ }; // Back-Bottom-Left
	Vertices[vertexIndex++] = { MaxX, MinY, MaxZ }; // Back-Bottom-Right
	Vertices[vertexIndex++] = { MaxX, MaxY, MaxZ }; // Back-Top-Right
	Vertices[vertexIndex++] = { MinX, MaxY, MaxZ }; // Back-Top-Left

	RenderedAABBoxInfo = boundingBoxInfo;
}

void UBoundingBoxLines::UpdateVertices(FOBB boundingBoxInfo)
{
	auto MatrixNearEqual = [](const FMatrix& A, const FMatrix& B, float eps = 1e-6f)
	{
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				if (fabs(A.Data[r][c] - B.Data[r][c]) > eps)
					return false;
		return true;
	};

    if (RenderedOBBoxInfo.Center   == boundingBoxInfo.Center &&
        RenderedOBBoxInfo.Extents  == boundingBoxInfo.Extents &&
        MatrixNearEqual(RenderedOBBoxInfo.Orientation, boundingBoxInfo.Orientation))
    {
        return;
    }

    if (Vertices.size() < NumVertices)
        Vertices.resize(NumVertices);

    const FVector  C = boundingBoxInfo.Center;
    const FVector  E = boundingBoxInfo.Extents;
    const FMatrix& M = boundingBoxInfo.Orientation;

    // Row-major + row-vector convention: rows are the world-space axes of the OBB.
    FVector Ux(M.Data[0][0], M.Data[0][1], M.Data[0][2]); // local +X axis in world
    FVector Uy(M.Data[1][0], M.Data[1][1], M.Data[1][2]); // local +Y axis in world
    FVector Uz(M.Data[2][0], M.Data[2][1], M.Data[2][2]); // local +Z axis in world

    // Be defensive in case the matrix isn’t perfectly orthonormal
    Ux.Normalize(); Uy.Normalize(); Uz.Normalize();

    // Scale axes by half-sizes
    const FVector ex = Ux * E.X;
    const FVector ey = Uy * E.Y;
    const FVector ez = Uz * E.Z;

    // Same corner ordering as your AABB version, but in OBB local ±X/±Y/±Z space:
    // 0~3: "front" face (using -Z side), 4~7: "back" face (using +Z side)
    uint32 i = 0;
    Vertices[i++] = C - ex - ey - ez; // Front-Bottom-Left
    Vertices[i++] = C + ex - ey - ez; // Front-Bottom-Right
    Vertices[i++] = C + ex + ey - ez; // Front-Top-Right
    Vertices[i++] = C - ex + ey - ez; // Front-Top-Left
    Vertices[i++] = C - ex - ey + ez; // Back-Bottom-Left
    Vertices[i++] = C + ex - ey + ez; // Back-Bottom-Right
    Vertices[i++] = C + ex + ey + ez; // Back-Top-Right
    Vertices[i++] = C - ex + ey + ez; // Back-Top-Left

    RenderedOBBoxInfo = boundingBoxInfo;
}
