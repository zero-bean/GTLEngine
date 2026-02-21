#pragma once
#include "fbxsdk.h"
#include <filesystem>
#include "Global/Vector.h"
#include "Global/Types.h"
#include "Core/Public/Archive.h"

struct FFbxMaterialInfo
{
	std::string MaterialName;
	std::filesystem::path DiffuseTexturePath;
	std::filesystem::path NormalTexturePath;
};

struct FFbxMeshSection
{
	uint32 StartIndex;
	uint32 IndexCount;
	uint32 MaterialIndex;
};

struct FFbxStaticMeshInfo
{
	TArray<FVector> VertexList;
	TArray<FVector> NormalList;
	TArray<FVector2> TexCoordList;
	TArray<FVector4> TangentList;  // XYZ: Tangent, W: Handedness
	TArray<uint32> Indices;

	TArray<FFbxMaterialInfo> Materials;
	TArray<FFbxMeshSection> Sections;
};

// ========================================
// 🔸 스켈레탈 메시 전용 구조체
// ========================================

/** FBX에서 추출한 본 정보 (엔진 독립적) */
struct FFbxBoneInfo
{
	std::string BoneName;
	int32 ParentIndex;  // -1이면 루트
	FTransform LocalTransform;  // 부모 기준 로컬 변환

	FFbxBoneInfo()
		: BoneName("")
		, ParentIndex(-1)
		, LocalTransform()
	{}
};

/** FBX에서 추출한 본 영향력 정보 (엔진 독립적) */
struct FFbxBoneInfluence
{
	static constexpr uint32 MAX_INFLUENCES = 12;

	/** 영향을 주는 본의 인덱스들 */
	int32 BoneIndices[MAX_INFLUENCES];

	/** 각 본의 가중치 (0~255, 합이 255) */
	uint8 BoneWeights[MAX_INFLUENCES];

	FFbxBoneInfluence()
	{
		for (int i = 0; i < MAX_INFLUENCES; ++i)
		{
			BoneIndices[i] = -1;
			BoneWeights[i] = 0;
		}
	}
};

/** 스켈레탈 메시 전용 데이터 (FFbxStaticMeshInfo를 상속하여 중복 제거) */
struct FFbxSkeletalMeshInfo : public FFbxStaticMeshInfo
{
	FName PathFileName;

	// 스켈레탈 전용 데이터 (FBX 전용 타입 사용)
	TArray<FFbxBoneInfo> Bones;              // 본 계층 구조
	TArray<FFbxBoneInfluence> SkinWeights;   // 정점별 스킨 가중치 (VertexList와 1:1 대응)
	TArray<int32> ControlPointIndices;       // 각 PolygonVertex가 어떤 ControlPoint에서 왔는지 매핑 (VertexList와 1:1 대응)
};

enum class EFbxMeshType
{
	Static,
	Skeletal,
	Unknown
};

class FFbxImporter
{
public:
	struct Configuration
	{
		bool bIsBinaryEnabled = false;
	};

	// 🔸 FBX SDK 세션 관리
	static bool Initialize();
	static void Shutdown();

	// 🔸 Public API - 타입별 로드 함수

	/** FBX 파일에서 메시 타입 판단 */
	static EFbxMeshType DetermineMeshType(const std::filesystem::path& FilePath);

	/** 스태틱 메시 임포트 */
	static bool LoadStaticMesh(
		const std::filesystem::path& FilePath,
		FFbxStaticMeshInfo* OutMeshInfo,
		Configuration Config = {});

	/** 스켈레탈 메시 임포트 */
	static bool LoadSkeletalMesh(
		const std::filesystem::path& FilePath,
		FFbxSkeletalMeshInfo* OutMeshInfo,
		Configuration Config = {});

private:
	// 🔸 RAII Helper - FbxScene 자동 관리 (메모리 릭 방지)
	class FFbxSceneGuard
	{
	private:
		FFbxSceneGuard(const FFbxSceneGuard&) = delete;
		FFbxSceneGuard& operator=(const FFbxSceneGuard&) = delete;

		FbxScene* Scene;
	public:
		explicit FFbxSceneGuard(FbxScene* InScene) : Scene(InScene) {}

		~FFbxSceneGuard()
		{
			if (Scene) { Scene->Destroy(); }
		}

		FbxScene* Get() const { return Scene; }
		FbxScene* operator->() const { return Scene; }
	};

	// 🔸 공통 Helper 함수들 (Static/Skeletal 모두 사용)
	static FbxScene* ImportFbxScene(const std::filesystem::path& FilePath, bool bTriangulateScene = true, bool* OutNeedsWindingReversal = nullptr);
	static FbxMesh* FindFirstMesh(FbxNode* RootNode, FbxNode** OutNode);
	static std::filesystem::path ResolveTexturePath(const std::string& OriginalPath, const std::filesystem::path& FbxDirectory, const std::filesystem::path& FbxFilePath);

	/** Material 추출 (Static/Skeletal 공통, 오프셋 지원) */
	static void ExtractMaterials(FbxNode* Node, const std::filesystem::path& FbxFilePath, FFbxStaticMeshInfo* OutMeshInfo, uint32 MaterialOffset = 0);

	/** Mesh Section 생성 (Static/Skeletal 공통) */
	static void BuildMeshSections(const TArray<TArray<uint32>>& IndicesPerMaterial, FFbxStaticMeshInfo* OutMeshInfo);

	/** mesh 단위 최적화 유틸 */
	static bool HasAnySkinnedMesh(FbxNode * Root);
	static bool EnsureTriangleMesh(FbxMesh * &Mesh, FbxGeometryConverter & Converter);

	// 🔸 Static Mesh 전용
	static void ExtractVertices(FbxMesh* Mesh, FFbxStaticMeshInfo* OutMeshInfo, const Configuration& Config);
	static void ExtractGeometryData(FbxMesh* Mesh, FFbxStaticMeshInfo* OutMeshInfo, const Configuration& Config, bool bReverseWinding);

	// 🔸 Skeletal Mesh 전용
	static FbxMesh* FindFirstSkinnedMesh(FbxNode* RootNode, FbxNode** OutNode);
	static void FindAllSkinnedMeshes(FbxNode* RootNode, TArray<FbxNode*>& OutMeshNodes);
	static bool ExtractSkeleton(FbxScene* Scene, FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo);

	/** 스켈레탈 메시의 스킨 가중치 추출/추가 (오프셋 지원으로 Extract/Append 통합) */
	static bool ExtractSkinWeights(FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo, uint32 VertexOffset = 0, int32 ControlPointOffset = 0);

	/** 스켈레탈 메시의 지오메트리 추출/추가 (오프셋 지원으로 Extract/Append 통합) */
	static void ExtractSkeletalGeometryData(FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo, const Configuration& Config,
		uint32 VertexOffset = 0, uint32 MaterialOffset = 0, int32 ControlPointOffset = 0);

	static inline FbxManager* SdkManager = nullptr;
	static inline FbxIOSettings* IoSettings = nullptr;
};

// ========================================
// 🔸 Bake 시스템
// ========================================

inline FArchive& operator<<(FArchive& Ar, FFbxMaterialInfo& MaterialInfo)
{
	// std::string을 FString으로 변환해서 직렬화
	if (Ar.IsLoading()) {
		FString TempName;
		Ar << TempName;
		MaterialInfo.MaterialName = TempName;

		FString TempPath;
		Ar << TempPath;
		MaterialInfo.DiffuseTexturePath = TempPath;

		FString TempNormalPath;
		Ar << TempNormalPath;
		MaterialInfo.NormalTexturePath = TempNormalPath;
	}
	else {
		FString TempName = MaterialInfo.MaterialName;
		Ar << TempName;

		FString TempPath = MaterialInfo.DiffuseTexturePath.string();
		Ar << TempPath;

		FString TempNormalPath = MaterialInfo.NormalTexturePath.string();
		Ar << TempNormalPath;
	}
	return Ar;
}

inline FArchive& operator<<(FArchive& Ar, FFbxMeshSection& Section)
{
	Ar << Section.StartIndex;
	Ar << Section.IndexCount;
	Ar << Section.MaterialIndex;
	return Ar;
}

inline FArchive& operator<<(FArchive& Ar, FFbxStaticMeshInfo& MeshInfo)
{
	Ar << MeshInfo.VertexList;
	Ar << MeshInfo.NormalList;
	Ar << MeshInfo.TexCoordList;
	Ar << MeshInfo.TangentList;
	Ar << MeshInfo.Indices;
	Ar << MeshInfo.Materials;
	Ar << MeshInfo.Sections;
	return Ar;
}

// FFbxBoneInfo 직렬화
inline FArchive& operator<<(FArchive& Ar, FFbxBoneInfo& BoneInfo)
{
	if (Ar.IsLoading()) {
		FString TempName;
		Ar << TempName;
		BoneInfo.BoneName = TempName;
	}
	else {
		FString TempName = BoneInfo.BoneName;
		Ar << TempName;
	}

	Ar << BoneInfo.ParentIndex;
	Ar << BoneInfo.LocalTransform.Translation;
	Ar << BoneInfo.LocalTransform.Rotation;
	Ar << BoneInfo.LocalTransform.Scale;

	return Ar;
}

// FFbxBoneInfluence 직렬화
inline FArchive& operator<<(FArchive& Ar, FFbxBoneInfluence& Influence)
{
	for (int i = 0; i < FFbxBoneInfluence::MAX_INFLUENCES; ++i) {
		Ar << Influence.BoneIndices[i];
		Ar << Influence.BoneWeights[i];
	}
	return Ar;
}

// FFbxSkeletalMeshInfo 직렬화 (베이스 클래스 멤버 포함)
inline FArchive& operator<<(FArchive& Ar, FFbxSkeletalMeshInfo& MeshInfo)
{
	// 베이스 클래스(FFbxStaticMeshInfo) 멤버 직렬화
	FFbxStaticMeshInfo& BaseInfo = static_cast<FFbxStaticMeshInfo&>(MeshInfo);
	Ar << BaseInfo;

	// 스켈레탈 전용 멤버 직렬화
	Ar << MeshInfo.Bones;                   // 본 정보
	Ar << MeshInfo.SkinWeights;             // 스킨 가중치
	Ar << MeshInfo.ControlPointIndices;     // 컨트롤 포인트 매핑
	return Ar;
}
