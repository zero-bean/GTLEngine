#pragma once
#include "pch.h"
#include "Core/Public/Object.h"


enum class EPrimitiveType
{
	Sphere,
	Triangle,
	Cube,
	Arrow,
};

class UResourceManager : public UObject
{
public:

	static UResourceManager& GetInstance();
	void Initialize();

	void Release();

	TArray<FVertex>* GetVertexData(EPrimitiveType Type);
	ID3D11Buffer* GetVertexbuffer(EPrimitiveType Type);
	UINT GetNumVertices(EPrimitiveType Type);

private:
	UResourceManager() = default;
	~UResourceManager() = default;
	TMap<EPrimitiveType, ID3D11Buffer*> Vertexbuffers;
	TMap<EPrimitiveType, UINT> NumVertices;
	TMap<EPrimitiveType, TArray<FVertex>*> VertexDatas;
};
