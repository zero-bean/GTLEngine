#pragma once
#include "stdafx.h"
#include "Vector.h"
#include "UEngineStatics.h"

struct FVertexPosColor4;

struct FVertexPosColor
{
	float x, y, z;       // Position (3개)
	float r, g, b, a;    // Color (4개)
};

//정점 구조체 선언, 삼각형 배열 정의
// 1. Define the triangle vertices
struct FVertexPosColor4
{
	float x, y, z, w;       // Position (float4) - w 추가
	float r, g, b, a;       // Color (float4)

	// 변환 함수
	static TArray<FVertexPosColor4> ConvertVertexData(const FVertexPosColor* oldVertices, int32 count)
	{
		TArray<FVertexPosColor4> newVertices;
		newVertices.reserve(count);

		for (int32 i = 0; i < count; i++)
		{
			FVertexPosColor4 newVertex;

			// Position: w = 1.0f 추가
			newVertex.x = oldVertices[i].x;
			newVertex.y = oldVertices[i].y;
			newVertex.z = oldVertices[i].z;
			newVertex.w = 1.0f;  // 동차좌표 w 컴포넌트

			// Color: 그대로 복사
			newVertex.r = oldVertices[i].r;
			newVertex.g = oldVertices[i].g;
			newVertex.b = oldVertices[i].b;
			newVertex.a = oldVertices[i].a;

			newVertices.push_back(newVertex);
		}

		return newVertices;
	}

	FVector GetPosition() const
	{
		return {x, y, z};
	}

};

// 텍스처용
struct FVertexPosTexCoord
{
	float x, y, z;
	float u, v;

	FVertexPosColor4 ConvertToFVPC4() const
	{
		FVertexPosColor4 tmp;
		tmp.x = x; tmp.y = y; tmp.z = z; tmp.w = 1;
		tmp.r = 1; tmp.g = 1; tmp.b = 1; tmp.a = 1;
		return tmp;
	}
};

// 폰트용
struct FVertexPosTexCoordFont
{
	FVertexPosTexCoord data;
	float charCode;
};

// 라인용
struct FVertexPos
{
	float x, y, z;

};

// 폰트 편집용
struct FSlicedUV
{
public:
	FSlicedUV() : u0(0), v0(0), u1(1), v1(1) {}
	FSlicedUV(float inU0, float inV0, float inU1, float inV1)
		: u0(inU0), v0(inV0), u1(inU1), v1(inV1) {}

	float u0;
	float v0;
	float u1;
	float v1;
};
