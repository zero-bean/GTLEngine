#pragma once

#include "UMesh.h"
#include "URenderer.h"
#include "UEngineSubsystem.h"

class UMeshManager : public UEngineSubsystem
{
	DECLARE_UCLASS(UMeshManager, UEngineSubsystem)
private:
	TMap<FString, UMesh*> meshes;

	/* *
	* @brief - Vertex 정보만 가지고 있는 Mesh를 생성할 때 사용합니다.
	*/
	UMesh* CreateMeshInternal(const TArray<FVertexPosColor>& vertices,
		D3D_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	/* *
	* @brief - Vertex 및 Index 정보를 가지고 있는 Mesh를 생성할 때 사용합니다.
	*/
	UMesh* CreateMeshInternal(const TArray<FVertexPosColor>& vertices, const TArray<uint32>& indices,
		D3D_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	/* *
	* @brief - Vertex(Pos, UV) 및 Index 정보를 가지고 있는 Mesh를 생성할 때 사용합니다.
	*/
	UMesh* CreateMeshInternal(const TArray<FVertexPosTexCoord>& vertices, const TArray<uint32>& indices,
		D3D_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
public:
	UMeshManager();
	~UMeshManager();

	bool Initialize(URenderer* renderer);
	UMesh* RetrieveMesh(FString meshName);
	bool RegisterMesh(const FString& name, UMesh* mesh) 
	{
		if (!mesh) return false;
		meshes[name] = mesh;
		return true;
	}
};
