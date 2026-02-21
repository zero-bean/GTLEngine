// UMesh.h
#pragma once
#include "stdafx.h"
#include "VertexPosColor.h"
#include "UObject.h"
#include "Vector4.h"
#include "BoundingBox.h"
struct FVertexPosColor4; // 전방 선언
//struct FVertexPosTexCoord;

enum class EVertexType {
	VERTEX_POS_COLOR,
	VERTEX_POS_UV
};

class UMesh : public UObject
{
	DECLARE_UCLASS(UMesh, UObject)
private:
	bool isInitialized = false;
	// FBounding Box 관련
	FBoundingBox PrecomputedLocalBox = FBoundingBox::Empty();
	bool bHasPrecomputedAABB = false;
	void ComputeLocalAABBFromVertices();
public:
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	TArray<FVertexPosColor4> Vertices;
	TArray<FVertexPosTexCoord> VerticesPosTexCoord;
	TArray<uint32> Indices;
	int32 NumVertices = 0;
	uint32 NumIndices = 0;
	D3D_PRIMITIVE_TOPOLOGY PrimitiveType;
	UINT Stride = 0;
	EVertexType Type = EVertexType::VERTEX_POS_COLOR;

	UMesh();
	// 생성자에서 초기화 리스트와 버텍스 버퍼를 생성
	UMesh(const TArray<FVertexPosColor4>& vertices, D3D_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	/* *
	* @brief - Vertex 및 Index 정보를 가지는 UMesh 생성자
	*/
	UMesh(const TArray<FVertexPosColor4>& vertices, const TArray<uint32>& indices,
		D3D_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	/* *
	* @brief - Vertex(Pos, UV) 및 Index 정보를 가지는 UMesh 생성자
	*/
	UMesh(const TArray<FVertexPosTexCoord>& vertices, const TArray<uint32>& indices,
		D3D_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	virtual ~UMesh() override;

	void Init(ID3D11Device* device);

	bool IsInitialized() const { return isInitialized; }
	// 로컬 AABB 접근자
	bool HasPrecomputedAABB() const { return bHasPrecomputedAABB; }
	const FBoundingBox& GetPrecomputedLocalBox() const { return PrecomputedLocalBox; }

};
