#pragma once
#include "pch.h"
#include "Core/Public/Object.h"

class UResourceManager : public UObject
{
	DECLARE_SINGLETON(UResourceManager)
public:
	void Initialize();

	void Release();

	TArray<FVertex>* GetVertexData(EPrimitiveType Type);
	ID3D11Buffer* GetVertexbuffer(EPrimitiveType Type);
	UINT GetNumVertices(EPrimitiveType Type);

private:

	TMap<EPrimitiveType, ID3D11Buffer*> Vertexbuffers;
	TMap<EPrimitiveType, UINT> NumVertices;
	TMap<EPrimitiveType, TArray<FVertex>*> VertexDatas;
};
