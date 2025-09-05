#pragma once
#include "VertexDatas.h"
#include "../Engine/pch.h"


enum class EPrimitiveType
{
	Sphere,
	Triangle,
	Cube,
};

class UResourceManager : public UObject
{
public:
	
	static UResourceManager& GetInstance();
	void Initialize(URenderer& Renderer);
	void Release(URenderer& Renderer);

	TArray<FVertexSimple>* GetVertexData(EPrimitiveType Type);
	ID3D11Buffer* GetVertexbuffer(EPrimitiveType Type);
	UINT GetNumVertices(EPrimitiveType Type);

private:
	UResourceManager() = default;
	~UResourceManager() = default;
	TMap<EPrimitiveType, ID3D11Buffer*> Vertexbuffers;
	TMap<EPrimitiveType, UINT> NumVertices;
	TMap<EPrimitiveType, TArray<FVertexSimple>*> VertexDatas;

};
