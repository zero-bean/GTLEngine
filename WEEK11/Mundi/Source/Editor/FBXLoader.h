#pragma once
#include "Object.h"
#include "fbxsdk.h"

class UAnimSequence;

class UFbxLoader : public UObject
{
public:

	DECLARE_CLASS(UFbxLoader, UObject)
	static UFbxLoader& GetInstance();
	UFbxLoader();

	static void PreLoad();

	FSkeletalMeshData* LoadFbxMeshAsset(const FString& FilePath);

	// 한 번의 FBX 파싱으로 메시와 애니메이션을 동시에 로드
	void LoadFbxAsset(const FString& FilePath);

protected:
	~UFbxLoader() override;
private:
	UFbxLoader(const UFbxLoader&) = delete;
	UFbxLoader& operator=(const UFbxLoader&) = delete;


	void LoadMeshFromNode(FbxNode* InNode, FSkeletalMeshData& MeshData, TMap<int32, TArray<uint32>>& MaterialGroupIndexList, TMap<FbxNode*, int32>& BoneToIndex, TMap<FbxSurfaceMaterial*, int32>& MaterialToIndex);

	void LoadSkeletonFromNode(FbxNode* InNode, FSkeletalMeshData& MeshData, int32 ParentNodeIndex, TMap<FbxNode*, int32>& BoneToIndex);

	void LoadMesh(FbxMesh* InMesh, FSkeletalMeshData& MeshData, TMap<int32, TArray<uint32>>& MaterialGroupIndexList, TMap<FbxNode*, int32>& BoneToIndex, TArray<int32> MaterialSlotToIndex, int32 DefaultMaterialIndex = 0);

	void ParseMaterial(FbxSurfaceMaterial* Material, FMaterialInfo& MaterialInfo);

	FString ParseTexturePath(FbxProperty& Property);

	void EnsureSingleRootBone(FSkeletalMeshData& MeshData);

	// 메시 노드 존재 여부를 재귀적으로 체크
	bool HasMeshNodesRecursive(FbxNode* Node);

	// bin파일 저장용
	TArray<FMaterialInfo> MaterialInfos;
	FbxManager* SdkManager = nullptr;
	
};
