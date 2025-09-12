#include "stdafx.h"
#include "UMesh.h"
#include "FVertexPosColor.h"
#include "UObject.h"
#include "UClass.h"

/* *
* D3D11_BUFFER_DESC https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ns-d3d11-d3d11_buffer_desc
* D3D11_SUBRESOURCE_DATA https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ns-d3d11-d3d11_subresource_data
*/

IMPLEMENT_UCLASS(UMesh, UObject)

UMesh::UMesh()
{

}

UMesh::UMesh(const TArray<FVertexPosColor4>& vertices, D3D_PRIMITIVE_TOPOLOGY primitiveType)
	: Vertices(vertices), PrimitiveType(primitiveType), NumVertices(vertices.size()), Stride(sizeof(FVertexPosColor4))
{

}

UMesh::UMesh(const TArray<FVertexPosColor4>& vertices, const TArray<uint32>& indices, D3D_PRIMITIVE_TOPOLOGY primitiveType)
	: Vertices(vertices), Indices(indices), PrimitiveType(primitiveType), NumVertices(vertices.size()), NumIndices(indices.size()), Stride(sizeof(FVertexPosColor4))
{

}

UMesh::~UMesh()
{
	if (VertexBuffer) { VertexBuffer->Release(); }
	if (IndexBuffer) { IndexBuffer->Release(); }
}

void UMesh::Init(ID3D11Device* device) 
{
    /* *
    * @brief - Vertex Buffer 생성
    */
    {
        // 정점 버퍼를 생성합니다 
        D3D11_BUFFER_DESC BufferDesc = {};
        BufferDesc.Usage = D3D11_USAGE_DEFAULT;
        BufferDesc.ByteWidth = static_cast<UINT>(Stride * NumVertices);
        BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        // 버퍼에 넣을 데이터를 정의합니다.
        D3D11_SUBRESOURCE_DATA BufferData = {};
        BufferData.pSysMem = Vertices.data();

        HRESULT hr = device->CreateBuffer(&BufferDesc, &BufferData, &VertexBuffer);
        // DEBUG 
        //assert(SUCCEEDED(hr), "UMESH-Init(): 버퍼 생성 실패");
    }
    /* *
    * @brief - Index Buffer 생성
    */
    if (NumIndices > 0)
    {
        // 정점 버퍼를 생성합니다 
        D3D11_BUFFER_DESC BufferDesc = {};
        BufferDesc.Usage = D3D11_USAGE_DEFAULT;
        BufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32) * NumIndices);
        BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        // 버퍼에 넣을 데이터를 정의합니다.
        D3D11_SUBRESOURCE_DATA BufferData = {};
        BufferData.pSysMem = Indices.data();

        HRESULT hr = device->CreateBuffer(&BufferDesc, &BufferData, &IndexBuffer);
        // DEBUG 
        //assert(SUCCEEDED(hr), "UMESH-Init(): 버퍼 생성 실패");
    }

	isInitialized = true;
}