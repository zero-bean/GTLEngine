// UMesh.h
#pragma once
#include "stdafx.h"
#include "FVertexPosColor.h"
#include "UObject.h"
#include "Vector4.h"

struct FVertexPosColor4; // 전방 선언

class UMesh : public UObject
{
	DECLARE_UCLASS(UMesh, UObject)
private:
	bool isInitialized = false;
public:
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	TArray<FVertexPosColor4> Vertices;
	TArray<uint32> Indices;
	int32 NumVertices = 0;
	uint32 NumIndices = 0;
	D3D_PRIMITIVE_TOPOLOGY PrimitiveType;
	UINT Stride = 0;

	UMesh();
	// 생성자에서 초기화 리스트와 버텍스 버퍼를 생성
	UMesh(const TArray<FVertexPosColor4>& vertices, D3D_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	/* *
	* @brief - Vertex 및 Index 정보를 가지는 UMesh 기본 생성자
	*/
	UMesh(const TArray<FVertexPosColor4>& vertices, const TArray<uint32>& indices,
		D3D_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	virtual ~UMesh() override;

	void Init(ID3D11Device* device);

	bool IsInitialized() const { return isInitialized; }

};
