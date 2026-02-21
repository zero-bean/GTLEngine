#include "pch.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Component/Mesh/Public/VertexDatas.h"
#include "Physics/Public/AABB.h"
#include "Texture/Public/Texture.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Manager/Asset/Public/FbxManager.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"

IMPLEMENT_SINGLETON_CLASS(UAssetManager, UObject)
UAssetManager::UAssetManager()
{
	TextureManager = new FTextureManager();
}

UAssetManager::~UAssetManager() = default;

void UAssetManager::Initialize()
{
	TextureManager->LoadAllTexturesFromDirectory(UPathManager::GetInstance().GetDataPath());
	// Data 폴더 속 모든 .obj 파일 로드 및 캐싱
	LoadAllObjStaticMesh();
	FFbxImporter::Initialize();
	LoadAllFbxMesh();  // FBX 통합 로드 (Static + Skeletal)

	VertexDatas.Emplace(EPrimitiveType::Torus, &VerticesTorus);
	VertexDatas.Emplace(EPrimitiveType::Arrow, &VerticesArrow);
	VertexDatas.Emplace(EPrimitiveType::CubeArrow, &VerticesCubeArrow);
	VertexDatas.Emplace(EPrimitiveType::Ring, &VerticesRing);
	VertexDatas.Emplace(EPrimitiveType::Line, &VerticesLine);
	VertexDatas.Emplace(EPrimitiveType::Sprite, &VerticesVerticalSquare);

	IndexDatas.Emplace(EPrimitiveType::Sprite, &IndicesVerticalSquare);
	IndexBuffers.Emplace(EPrimitiveType::Sprite,
		FRenderResourceFactory::CreateIndexBuffer(IndicesVerticalSquare.GetData(), static_cast<int>(IndicesVerticalSquare.Num()) * sizeof(uint32)));

	NumIndices.Emplace(EPrimitiveType::Sprite, static_cast<uint32>(IndicesVerticalSquare.Num()));

	VertexBuffers.Emplace(EPrimitiveType::Torus, FRenderResourceFactory::CreateVertexBuffer(
		VerticesTorus.GetData(), static_cast<int>(VerticesTorus.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::Arrow, FRenderResourceFactory::CreateVertexBuffer(
		VerticesArrow.GetData(), static_cast<int>(VerticesArrow.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::CubeArrow, FRenderResourceFactory::CreateVertexBuffer(
		VerticesCubeArrow.GetData(), static_cast<int>(VerticesCubeArrow.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::Ring, FRenderResourceFactory::CreateVertexBuffer(
		VerticesRing.GetData(), static_cast<int>(VerticesRing.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::Line, FRenderResourceFactory::CreateVertexBuffer(
		VerticesLine.GetData(), static_cast<int>(VerticesLine.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::Sprite, FRenderResourceFactory::CreateVertexBuffer(
		VerticesVerticalSquare.GetData(), static_cast<int>(VerticesVerticalSquare.Num() * sizeof(FNormalVertex))));

	NumVertices.Emplace(EPrimitiveType::Torus, static_cast<uint32>(VerticesTorus.Num()));
	NumVertices.Emplace(EPrimitiveType::Arrow, static_cast<uint32>(VerticesArrow.Num()));
	NumVertices.Emplace(EPrimitiveType::CubeArrow, static_cast<uint32>(VerticesCubeArrow.Num()));
	NumVertices.Emplace(EPrimitiveType::Ring, static_cast<uint32>(VerticesRing.Num()));
	NumVertices.Emplace(EPrimitiveType::Line, static_cast<uint32>(VerticesLine.Num()));
	NumVertices.Emplace(EPrimitiveType::Sprite, static_cast<uint32>(VerticesVerticalSquare.Num()));

	// Calculate AABB for all primitive types (excluding StaticMesh)
	for (const auto& Pair : VertexDatas)
	{
		EPrimitiveType Type = Pair.first;
		const auto* Vertices = Pair.second;
		if (!Vertices || Vertices->IsEmpty())
		{
			continue;
		}

		AABBs[Type] = CalculateAABB(*Vertices);
	}

	// Calculate AABB for each StaticMesh
	for (const auto& MeshPair : StaticMeshCache)
	{
		const FName& ObjPath = MeshPair.first;
		const auto& Mesh = MeshPair.second;
		if (!Mesh || !Mesh->IsValid())
		{
			continue;
		}

		const auto& Vertices = Mesh->GetVertices();
		if (Vertices.IsEmpty())
		{
			continue;
		}

		StaticMeshAABBs[ObjPath] = CalculateAABB(Vertices);
	}
}

void UAssetManager::Release()
{
	// TMap.Value()
	for (auto& Pair : VertexBuffers)
	{
		SafeRelease(Pair.second);
	}
	for (auto& Pair : IndexBuffers)
	{
		SafeRelease(Pair.second);
	}

	for (auto& Pair : StaticMeshVertexBuffers)
	{
		SafeRelease(Pair.second);
	}
	for (auto& Pair : StaticMeshIndexBuffers)
	{
		SafeRelease(Pair.second);
	}

	for (auto& Pair : SkeletalMeshVertexBuffers)
	{
		SafeRelease(Pair.second);
	}
	for (auto& Pair : SkeletalMeshIndexBuffers)
	{
		SafeRelease(Pair.second);
	}

	// Delete UStaticMesh objects
	for (auto& Pair : StaticMeshCache)
	{
		SafeDelete(Pair.second);
	}
	StaticMeshCache.Empty();
	StaticMeshVertexBuffers.Empty();
	StaticMeshIndexBuffers.Empty();

	// Delete USkeletalMesh objects
	for (auto& Pair : SkeletalMeshCache)
	{
		SafeDelete(Pair.second);
	}
	SkeletalMeshCache.Empty();
	SkeletalMeshVertexBuffers.Empty();
	SkeletalMeshIndexBuffers.Empty();

	// TMap.Empty()
	VertexBuffers.Empty();
	IndexBuffers.Empty();

	SafeDelete(TextureManager);

	// Release manager caches
	FFbxImporter::Shutdown();
	FObjManager::Release();
	FFbxManager::Release();
}

/**
 * @brief Data/ 경로 하위에 모든 .obj 파일을 로드 후 캐싱한다
 */
void UAssetManager::LoadAllObjStaticMesh()
{
	TArray<FName> ObjList;
	const FString DataDirectory = "Data/"; // 검색할 기본 디렉토리
	// 디렉토리가 실제로 존재하는지 먼저 확인합니다.
	if (std::filesystem::exists(DataDirectory) && std::filesystem::is_directory(DataDirectory))
	{
		// recursive_directory_iterator를 사용하여 디렉토리와 모든 하위 디렉토리를 순회합니다.
		for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataDirectory))
		{
			// 현재 항목이 일반 파일이고, 확장자가 ".obj"인지 확인합니다.
			if (Entry.is_regular_file() && Entry.path().extension() == ".obj")
			{
				// .generic_string()을 사용하여 OS에 상관없이 '/' 구분자를 사용하는 경로를 바로 얻습니다.
				FString PathString = Entry.path().generic_string();

				// 찾은 파일 경로를 FName으로 변환하여 ObjList에 추가합니다.
				ObjList.Emplace(FName(PathString));
			}
		}
	}

	// Enable winding order flip for this OBJ file
	FObjImporter::Configuration Config;
	Config.bFlipWindingOrder = false;
	Config.bIsBinaryEnabled = true;
	Config.bPositionToUEBasis = true;
	Config.bNormalToUEBasis = true;
	Config.bUVToUEBasis = true;

	// 범위 기반 for문을 사용하여 배열의 모든 요소를 순회합니다.
	for (const FName& ObjPath : ObjList)
	{
		// FObjManager가 UStaticMesh 포인터를 반환한다고 가정합니다.
		UStaticMesh* LoadedMesh = FObjManager::LoadObjStaticMesh(ObjPath, Config);

		// 로드에 성공했는지 확인합니다.
		if (LoadedMesh)
		{
			StaticMeshCache.Emplace(ObjPath, LoadedMesh);

			StaticMeshVertexBuffers.Emplace(ObjPath, this->CreateVertexBuffer(LoadedMesh->GetVertices()));
			StaticMeshIndexBuffers.Emplace(ObjPath, this->CreateIndexBuffer(LoadedMesh->GetIndices()));
		}
	}
}

void UAssetManager::LoadAllFbxMesh()
{
	TArray<FName> FbxList;
	const FString DataDirectory = "Data/";

	if (std::filesystem::exists(DataDirectory))
	{
		for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataDirectory))
		{
			if (Entry.is_regular_file() && Entry.path().extension() == ".fbx")
			{
				FbxList.Emplace(FName(Entry.path().generic_string()));
			}
		}
	}

	FFbxImporter::Configuration Config;
	Config.bIsBinaryEnabled = true;

	// 1. FBX 파일 순회 로드
	for (const FName& FbxPath : FbxList)
	{
		UObject* LoadedObject = FFbxManager::LoadFbxMesh(FbxPath, Config);
		if (!LoadedObject)
		{
			UE_LOG_ERROR("FBX 로드 실패: %s", FbxPath.ToString().c_str());
			continue;
		}

		// Static Mesh 타입이면 바로 등록
		if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(LoadedObject))
		{
			StaticMeshCache.Emplace(FbxPath, StaticMesh);
			StaticMeshVertexBuffers.Emplace(FbxPath, CreateVertexBuffer(StaticMesh->GetVertices()));
			StaticMeshIndexBuffers.Emplace(FbxPath, CreateIndexBuffer(StaticMesh->GetIndices()));

			UE_LOG_SUCCESS("[UAssetManager] FBX StaticMesh 로드 완료: %s", FbxPath.ToString().c_str());
		}
		// Skeletal Mesh 타입이면 캐싱 + StaticMesh 캐싱 병행
		else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(LoadedObject))
		{
			AddSkeletalMeshToCache(FbxPath, SkeletalMesh);
			UE_LOG_SUCCESS("[UAssetManager] FBX SkeletalMesh 로드 완료: %s", FbxPath.ToString().c_str());

			// 내부 StaticMesh를 StaticMeshCache에도 등록 (스태틱 렌더링용)
			if (UStaticMesh* InnerStaticMesh = SkeletalMesh->GetStaticMesh())
			{
				StaticMeshCache.Emplace(FbxPath, InnerStaticMesh);
				StaticMeshVertexBuffers.Emplace(FbxPath, CreateVertexBuffer(InnerStaticMesh->GetVertices()));
				StaticMeshIndexBuffers.Emplace(FbxPath, CreateIndexBuffer(InnerStaticMesh->GetIndices()));

				UE_LOG_SUCCESS("[UAssetManager] FBX SkeletalMesh → StaticMesh 캐싱 완료: %s", FbxPath.ToString().c_str());
			}
			else
			{
				UE_LOG_WARNING("[UAssetManager] SkeletalMesh에 StaticMesh 데이터가 없습니다: %s", FbxPath.ToString().c_str());
			}
		}
		else
		{
			UE_LOG_WARNING("[UAssetManager] 알 수 없는 FBX 타입: %s", FbxPath.ToString().c_str());
		}
	}

	// 2. 스태틱 메시 AABB 계산 (스켈레탈 내부 메시 포함)
	for (const auto& MeshPair : StaticMeshCache)
	{
		const FName& ObjPath = MeshPair.first;
		UStaticMesh* StaticMesh = MeshPair.second;

		if (!StaticMesh || !StaticMesh->IsValid())
			continue;

		const auto& Vertices = StaticMesh->GetVertices();
		if (Vertices.IsEmpty())
			continue;

		StaticMeshAABBs[ObjPath] = CalculateAABB(Vertices);
	}
}

ID3D11Buffer* UAssetManager::GetVertexBuffer(FName InObjPath)
{
	return StaticMeshVertexBuffers.FindRef(InObjPath);
}

ID3D11Buffer* UAssetManager::GetIndexBuffer(FName InObjPath)
{
	return StaticMeshIndexBuffers.FindRef(InObjPath);
}

ID3D11Buffer* UAssetManager::CreateVertexBuffer(TArray<FNormalVertex> InVertices)
{
	return FRenderResourceFactory::CreateVertexBuffer(InVertices.GetData(), static_cast<int>(InVertices.Num()) * sizeof(FNormalVertex));
}

ID3D11Buffer* UAssetManager::CreateIndexBuffer(TArray<uint32> InIndices)
{
	return FRenderResourceFactory::CreateIndexBuffer(InIndices.GetData(), static_cast<int>(InIndices.Num()) * sizeof(uint32));
}

TArray<FNormalVertex>* UAssetManager::GetVertexData(EPrimitiveType InType)
{
	return VertexDatas[InType];
}

ID3D11Buffer* UAssetManager::GetVertexbuffer(EPrimitiveType InType)
{
	return VertexBuffers[InType];
}

uint32 UAssetManager::GetNumVertices(EPrimitiveType InType)
{
	return NumVertices[InType];
}

TArray<uint32>* UAssetManager::GetIndexData(EPrimitiveType InType)
{
	return IndexDatas[InType];
}

ID3D11Buffer* UAssetManager::GetIndexBuffer(EPrimitiveType InType)
{
	return IndexBuffers[InType];
}

uint32 UAssetManager::GetNumIndices(EPrimitiveType InType)
{
	return NumIndices[InType];
}

FAABB& UAssetManager::GetAABB(EPrimitiveType InType)
{
	return AABBs[InType];
}

FAABB& UAssetManager::GetStaticMeshAABB(FName InName)
{
	return StaticMeshAABBs[InName];
}

// StaticMesh Cache Accessors
UStaticMesh* UAssetManager::GetStaticMeshFromCache(const FName& InObjPath)
{
	if (auto* FoundPtr = StaticMeshCache.Find(InObjPath))
	{
		return *FoundPtr;
	}
	return nullptr;
}

void UAssetManager::AddStaticMeshToCache(const FName& InObjPath, UStaticMesh* InStaticMesh)
{
	if (!InStaticMesh)
	{
		return;
	}

	if (!StaticMeshCache.Contains(InObjPath))
	{
		StaticMeshCache.Add(InObjPath, InStaticMesh);
	}
}

// SkeletalMesh Cache Accessors
USkeletalMesh* UAssetManager::GetSkeletalMeshFromCache(const FName& InFbxPath)
{
	if (auto* FoundPtr = SkeletalMeshCache.Find(InFbxPath))
	{
		return *FoundPtr;
	}
	return nullptr;
}

void UAssetManager::AddSkeletalMeshToCache(const FName& InFbxPath, USkeletalMesh* InSkeletalMesh)
{
	if (!InSkeletalMesh)
	{
		return;
	}

	if (!SkeletalMeshCache.Contains(InFbxPath))
	{
		SkeletalMeshCache.Add(InFbxPath, InSkeletalMesh);
	}
}

/**
 * @brief Vertex 배열로부터 AABB(Axis-Aligned Bounding Box)를 계산하는 헬퍼 함수
 * @param Vertices 정점 데이터 배열
 * @return 계산된 FAABB 객체
 */
FAABB UAssetManager::CalculateAABB(const TArray<FNormalVertex>& Vertices)
{
	FVector MinPoint(+FLT_MAX, +FLT_MAX, +FLT_MAX);
	FVector MaxPoint(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const auto& Vertex : Vertices)
	{
		MinPoint.X = std::min(MinPoint.X, Vertex.Position.X);
		MinPoint.Y = std::min(MinPoint.Y, Vertex.Position.Y);
		MinPoint.Z = std::min(MinPoint.Z, Vertex.Position.Z);

		MaxPoint.X = std::max(MaxPoint.X, Vertex.Position.X);
		MaxPoint.Y = std::max(MaxPoint.Y, Vertex.Position.Y);
		MaxPoint.Z = std::max(MaxPoint.Z, Vertex.Position.Z);
	}

	return FAABB(MinPoint, MaxPoint);
}

ID3D11Buffer* UAssetManager::GetSkeletalMeshVertexBuffer(const FName& InFbxPath)
{
	return SkeletalMeshVertexBuffers.FindRef(InFbxPath);
}

ID3D11Buffer* UAssetManager::GetSkeletalMeshIndexBuffer(const FName& InFbxPath)
{
	return SkeletalMeshIndexBuffers.FindRef(InFbxPath);
}

/**
 * @brief 넘겨준 경로로 캐싱된 UTexture 포인터를 반환해주는 함수
 * @param 로드할 텍스처 경로
 * @return 캐싱된 UTexture 포인터
 */
UTexture* UAssetManager::LoadTexture(const FName& InFilePath)
{
	return TextureManager->LoadTexture(InFilePath);
}

/**
 * @brief 지금까지 캐싱된 UTexture 포인터 목록 반환해주는 함수
 * @return {경로, 캐싱된 UTexture 포인터}
 */
const TMap<FName, UTexture*>& UAssetManager::GetTextureCache() const
{
	return TextureManager->GetTextureCache();
}
