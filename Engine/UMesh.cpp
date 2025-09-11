#include "stdafx.h"
#include "UMesh.h"
#include "FVertexPosColor.h"
#include "UObject.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UMesh, UObject)

UMesh::UMesh()
{
}

UMesh::UMesh(const TArray<FVertexPosColor4>& vertices, D3D_PRIMITIVE_TOPOLOGY primitiveType)
	: Vertices(vertices), PrimitiveType(primitiveType), NumVertices(static_cast<int32>(vertices.size())), Stride(sizeof(FVertexPosColor4))
{
}

void UMesh::Init(ID3D11Device* device) {
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(FVertexPosColor4) * NumVertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = Vertices.data();

	HRESULT hr = device->CreateBuffer(&vbd, &vertexData, &VertexBuffer);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create vertex buffer for mesh");
	}

	isInitialized = true;
}