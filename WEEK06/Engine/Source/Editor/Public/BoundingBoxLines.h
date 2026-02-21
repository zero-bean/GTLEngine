#pragma once
#include "Core/Public/Object.h"
#include "Physics/Public/AABB.h"
#include "Physics/Public/OBB.h"

class UBoundingBoxLines : UObject
{
public:
	UBoundingBoxLines();
	~UBoundingBoxLines() = default;

	void MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex);
	void UpdateVertices(FAABB boundingBoxInfo);
	void UpdateVertices(FOBB boundingBoxInfo);

	uint32 GetNumVertices() const
	{
		return NumVertices;
	}

	FAABB GetRenderedAABBoxInfo() const
	{
		return RenderedAABBoxInfo;
	}

	FOBB GetRenderedOBBoxInfo() const
	{
		return RenderedOBBoxInfo;
	}

private:
	TArray<FVector> Vertices;
	uint32 NumVertices = 8;
	FAABB RenderedAABBoxInfo;
	FOBB RenderedOBBoxInfo;
};

