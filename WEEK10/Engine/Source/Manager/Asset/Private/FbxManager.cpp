#include "pch.h"

#include "Manager/Asset/Public/FbxManager.h"
#include "Manager/Asset/Public/AssetManager.h"

// ========================================
// 🔸 Static member variable definition
// ========================================

TMap<FName, std::unique_ptr<FStaticMesh>> FFbxManager::FbxFStaticMeshMap;
TMap<FName, std::unique_ptr<FStaticMesh>> FFbxManager::FbxSkeletalFStaticMeshMap;

// ========================================
// 🔸 Public API
// ========================================

UObject* FFbxManager::LoadFbxMesh(const FName& FilePath, const FFbxImporter::Configuration& Config)
{
	// 메시 타입 판단
	EFbxMeshType MeshType = FFbxImporter::DetermineMeshType(FilePath.ToString());

	switch (MeshType)
	{
	case EFbxMeshType::Static:
		UE_LOG("[FbxManager] Static Mesh로 로드: %s", FilePath.ToString().c_str());
		return LoadFbxStaticMesh(FilePath, Config);

	case EFbxMeshType::Skeletal:
		UE_LOG("[FbxManager] Skeletal Mesh로 로드: %s", FilePath.ToString().c_str());
		return LoadFbxSkeletalMesh(FilePath, Config);

	case EFbxMeshType::Unknown:
	default:
		UE_LOG_ERROR("FBX 메시 타입을 판단할 수 없습니다: %s", FilePath.ToString().c_str());
		return nullptr;
	}
}

FStaticMesh* FFbxManager::LoadFbxStaticMeshAsset(const FName& FilePath, const FFbxImporter::Configuration& Config)
{
	// 캐시에서 먼저 찾기
	auto* FoundValuePtr = FbxFStaticMeshMap.Find(FilePath);
	if (FoundValuePtr)
	{
		return FoundValuePtr->get();
	}

	FFbxStaticMeshInfo MeshInfo;
	if (!FFbxImporter::LoadStaticMesh(FilePath.ToString(), &MeshInfo, Config))
	{
		UE_LOG_ERROR("FBX 로드 실패: %s", FilePath.ToString().c_str());
		return nullptr;
	}

	auto StaticMesh = std::make_unique<FStaticMesh>();
	StaticMesh->PathFileName = FilePath;

	ConvertFbxToStaticMesh(MeshInfo, StaticMesh.get());

	UE_LOG_SUCCESS("FBX StaticMesh 변환 완료: %s", FilePath.ToString().c_str());

	// 캐시에 저장
	FbxFStaticMeshMap.Emplace(FilePath, std::move(StaticMesh));
	return FbxFStaticMeshMap[FilePath].get();
}

UStaticMesh* FFbxManager::LoadFbxStaticMesh(const FName& FilePath, const FFbxImporter::Configuration& Config)
{
	FStaticMesh* StaticMeshAsset = LoadFbxStaticMeshAsset(FilePath, Config);
	if (!StaticMeshAsset)
		return nullptr;

	StaticMeshAsset->PathFileName = FilePath;

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
	StaticMesh->SetStaticMeshAsset(StaticMeshAsset);

	// Materials 생성 및 설정
	for (int32 i = 0; i < StaticMeshAsset->MaterialInfo.Num(); ++i)
	{
		// MaterialInfo를 복사해서 전달 (참조 문제 회피)
		FMaterial MaterialCopy = StaticMeshAsset->MaterialInfo[i];
		UMaterial* NewMaterial = CreateMaterialFromInfo(MaterialCopy, i);
		StaticMesh->SetMaterial(i, NewMaterial);
	}

	// 캐시에 등록
	UAssetManager::GetInstance().AddStaticMeshToCache(FilePath, StaticMesh);

	return StaticMesh;
}

// ========================================
// 🔸 Private Helper Functions
// ========================================

void FFbxManager::ConvertFbxToStaticMesh(const FFbxStaticMeshInfo& MeshInfo, FStaticMesh* OutStaticMesh)
{
	// Vertices 변환
	for (int i = 0; i < MeshInfo.VertexList.Num(); ++i)
	{
		FNormalVertex Vertex{};
		Vertex.Position = MeshInfo.VertexList[i];
		Vertex.Normal = MeshInfo.NormalList.IsValidIndex(i) ? MeshInfo.NormalList[i] : FVector(0, 1, 0);
		Vertex.TexCoord = MeshInfo.TexCoordList.IsValidIndex(i) ? MeshInfo.TexCoordList[i] : FVector2(0, 0);
		Vertex.Tangent = MeshInfo.TangentList.IsValidIndex(i) ? MeshInfo.TangentList[i] : FVector4(1, 0, 0, 1);
		OutStaticMesh->Vertices.Add(Vertex);
	}

	OutStaticMesh->Indices = MeshInfo.Indices;

	// Materials 변환
	for (const FFbxMaterialInfo& FbxMat : MeshInfo.Materials)
	{
		FMaterial Material{};
		Material.Name = FbxMat.MaterialName;
		Material.Kd = FVector(0.9f, 0.9f, 0.9f);
		Material.Ka = FVector(0.2f, 0.2f, 0.2f);
		Material.Ks = FVector(0.5f, 0.5f, 0.5f);
		Material.Ns = 32.0f;
		Material.D = 1.0f;

		if (!FbxMat.DiffuseTexturePath.empty())
		{
			Material.KdMap = FbxMat.DiffuseTexturePath.generic_string();
		}
		if (!FbxMat.NormalTexturePath.empty())
		{
			Material.NormalMap = FbxMat.NormalTexturePath.generic_string();
		}
		OutStaticMesh->MaterialInfo.Add(Material);
	}

	// Sections 변환
	for (const FFbxMeshSection& FbxSection : MeshInfo.Sections)
	{
		FMeshSection Section{};
		Section.StartIndex = FbxSection.StartIndex;
		Section.IndexCount = FbxSection.IndexCount;
		Section.MaterialSlot = FbxSection.MaterialIndex;
		OutStaticMesh->Sections.Add(Section);
	}
}

UMaterial* FFbxManager::CreateMaterialFromInfo(const FMaterial& MaterialInfo, int32 MaterialIndex)
{
	UMaterial* NewMaterial = NewObject<UMaterial>();
	NewMaterial->SetName(FName(MaterialInfo.Name));
	NewMaterial->SetMaterialData(MaterialInfo);

	// Diffuse Texture 로드
	if (!MaterialInfo.KdMap.empty())
	{
		UE_LOG("[FbxManager] Material %d - Diffuse Texture Path: %s", MaterialIndex, MaterialInfo.KdMap.c_str());

		UTexture* DiffuseTexture = UAssetManager::GetInstance().LoadTexture(FName(MaterialInfo.KdMap));
		if (DiffuseTexture)
		{
			NewMaterial->SetDiffuseTexture(DiffuseTexture);
			UE_LOG_SUCCESS("[FbxManager] Material %d - Diffuse Texture Loaded Successfully", MaterialIndex);
		}
		else
		{
			UE_LOG_ERROR("[FbxManager] Material %d - Diffuse Texture Load Failed: %s", MaterialIndex, MaterialInfo.KdMap.c_str());
		}
	}
	else
	{
		UE_LOG_WARNING("[FbxManager] Material %d - No Diffuse Texture Path", MaterialIndex);
	}

	// Normal Texture 로드
	if (!MaterialInfo.NormalMap.empty())
	{
		UE_LOG("[FbxManager] Material %d - Normal Map Path: %s", MaterialIndex, MaterialInfo.NormalMap.c_str());

		UTexture* NormalTexture = UAssetManager::GetInstance().LoadTexture(FName(MaterialInfo.NormalMap));
		if (NormalTexture)
		{
			NewMaterial->SetNormalTexture(NormalTexture);
			UE_LOG_SUCCESS("[FbxManager] Material %d - Normal Map Loaded Successfully", MaterialIndex);
		}
		else
		{
			UE_LOG_ERROR("[FbxManager] Material %d - Normal Map Load Failed: %s", MaterialIndex, MaterialInfo.NormalMap.c_str());
		}
	}

	return NewMaterial;
}

// ========================================
// 🔸 Skeletal Mesh Public API
// ========================================

FStaticMesh* FFbxManager::LoadFbxSkeletalMeshAsset(const FName& FilePath, const FFbxImporter::Configuration& Config)
{
	// 캐시에서 먼저 찾기
	auto* FoundValuePtr = FbxSkeletalFStaticMeshMap.Find(FilePath);
	if (FoundValuePtr)
	{
		return FoundValuePtr->get();
	}

	FFbxSkeletalMeshInfo SkeletalMeshInfo;
	if (!FFbxImporter::LoadSkeletalMesh(FilePath.ToString(), &SkeletalMeshInfo, Config))
	{
		UE_LOG_ERROR("FBX 스켈레탈 메시 로드 실패: %s", FilePath.ToString().c_str());
		return nullptr;
	}

	// FStaticMesh 생성 (지오메트리 데이터만)
	auto StaticMeshAsset = std::make_unique<FStaticMesh>();
	ConvertFbxSkeletalToStaticMesh(SkeletalMeshInfo, StaticMeshAsset.get());
	StaticMeshAsset->PathFileName = FilePath;

	UE_LOG_SUCCESS("FBX SkeletalMesh Asset 변환 완료: %s", FilePath.ToString().c_str());

	// 캐시에 저장
	FbxSkeletalFStaticMeshMap.Emplace(FilePath, std::move(StaticMeshAsset));
	return FbxSkeletalFStaticMeshMap[FilePath].get();
}

USkeletalMesh* FFbxManager::LoadFbxSkeletalMesh(const FName& FilePath, const FFbxImporter::Configuration& Config)
{
	// 1) AssetManager 캐시에서 먼저 찾기
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	if (USkeletalMesh* Cached = AssetManager.GetSkeletalMeshFromCache(FilePath))
	{
		return Cached;
	}

	// 2) FBX 스켈레탈 메시 정보 로드 (한 번만 로드)
	FFbxSkeletalMeshInfo SkeletalMeshInfo;
	if (!FFbxImporter::LoadSkeletalMesh(FilePath.ToString(), &SkeletalMeshInfo, Config))
	{
		UE_LOG_ERROR("FBX 스켈레탈 메시 로드 실패: %s", FilePath.ToString().c_str());
		return nullptr;
	}

	// 3) 이미 로드된 데이터로부터 FStaticMesh Asset 생성 (중복 로드 방지)
	FStaticMesh* StaticMeshAsset = GetOrCreateStaticMeshFromInfo(FilePath, SkeletalMeshInfo);
	if (!StaticMeshAsset)
	{
		UE_LOG_ERROR("FBX SkeletalMesh Asset 변환 실패: %s", FilePath.ToString().c_str());
		return nullptr;
	}

	// 4) USkeletalMesh 생성 및 설정
	USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>();
	if (!ConvertFbxToSkeletalMesh(SkeletalMeshInfo, SkeletalMesh, StaticMeshAsset))
	{
		UE_LOG_ERROR("FBX → SkeletalMesh 변환 실패: %s", FilePath.ToString().c_str());
		delete SkeletalMesh;
		return nullptr;
	}

	UE_LOG_SUCCESS("FBX SkeletalMesh 변환 완료: %s", FilePath.ToString().c_str());

	// 5) AssetManager에 등록
	AssetManager.AddSkeletalMeshToCache(FilePath, SkeletalMesh);

	return SkeletalMesh;
}

// ========================================
// 🔸 Skeletal Mesh Helper Functions
// ========================================

bool FFbxManager::ConvertFbxToSkeletalMesh(const FFbxSkeletalMeshInfo& FbxData, USkeletalMesh* OutSkeletalMesh, FStaticMesh* StaticMeshAsset)
{
	if (!OutSkeletalMesh)
	{
		UE_LOG_ERROR("유효하지 않은 SkeletalMesh입니다.");
		return false;
	}

	if (!StaticMeshAsset)
	{
		UE_LOG_ERROR("유효하지 않은 StaticMeshAsset입니다.");
		return false;
	}

	// 1. 스켈레톤 변환
	ConvertSkeleton(FbxData.Bones, OutSkeletalMesh->GetRefSkeleton());

	// 2. UStaticMesh 생성 및 설정 (지오메트리 데이터)
	// StaticMeshAsset는 이미 LoadFbxSkeletalMeshAsset()에서 생성 및 캐싱됨
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
	StaticMesh->SetStaticMeshAsset(StaticMeshAsset);

	// Materials 생성 및 설정
	for (int32 i = 0; i < StaticMeshAsset->MaterialInfo.Num(); ++i)
	{
		FMaterial MaterialCopy = StaticMeshAsset->MaterialInfo[i];
		UMaterial* NewMaterial = CreateMaterialFromInfo(MaterialCopy, i);
		StaticMesh->SetMaterial(i, NewMaterial);
	}

	OutSkeletalMesh->SetStaticMesh(StaticMesh);

	// 3. 렌더 데이터 생성 (스킨 가중치만)
	FSkeletalMeshRenderData* RenderData = new FSkeletalMeshRenderData();
	if (!RenderData)
	{
		UE_LOG_ERROR("SkeletalMeshRenderData가 없습니다.");
		return false;
	}

	// 4. 스킨 가중치 변환
	ConvertSkinWeights(FbxData.SkinWeights, RenderData->SkinWeightVertices);

	// 5. RenderData를 SkeletalMesh에 설정
	OutSkeletalMesh->SetSkeletalMeshRenderData(RenderData);

	// 6. Inverse Reference Matrices 계산
	OutSkeletalMesh->CalculateInvRefMatrices();

	UE_LOG_SUCCESS("[FbxManager] SkeletalMesh 변환 완료 - Bones: %d, Vertices: %d",
		FbxData.Bones.Num(), FbxData.VertexList.Num());

	return true;
}

void FFbxManager::ConvertSkeleton(const TArray<FFbxBoneInfo>& FbxBones, FReferenceSkeleton& OutRefSkeleton)
{
	TArray<FMeshBoneInfo> BoneInfos;
	TArray<FTransform> BonePoses;

	BoneInfos.Reserve(FbxBones.Num());
	BonePoses.Reserve(FbxBones.Num());

	for (int32 i = 0; i < FbxBones.Num(); ++i)
	{
		const FFbxBoneInfo& FbxBone = FbxBones[i];

		// FMeshBoneInfo 생성
		FMeshBoneInfo BoneInfo;
		BoneInfo.Name = FName(FbxBone.BoneName);
		BoneInfo.ParentIndex = FbxBone.ParentIndex;

		// FTransform은 그대로 사용
		FTransform BonePose = FbxBone.LocalTransform;

		BoneInfos.Add(BoneInfo);
		BonePoses.Add(BonePose);

		UE_LOG("[FbxManager] 본 %d 준비: %s (부모: %d)",
			i, FbxBone.BoneName.c_str(), FbxBone.ParentIndex);
	}

	// 한 번에 초기화
	OutRefSkeleton.InitializeFromData(BoneInfos, BonePoses);
	UE_LOG_SUCCESS("[FbxManager] ReferenceSkeleton 초기화 완료: %d 본", BoneInfos.Num());
}

void FFbxManager::ConvertSkinWeights(const TArray<FFbxBoneInfluence>& FbxWeights, TArray<FRawSkinWeight>& OutSkinWeights)
{
	//OutSkinWeights.Reset(FbxWeights.Num());
	OutSkinWeights.Empty(FbxWeights.Num());

	for (const FFbxBoneInfluence& FbxWeight : FbxWeights)
	{
		FRawSkinWeight SkinWeight;

		// FBX 가중치 → 엔진 가중치
		for (int32 i = 0; i < FFbxBoneInfluence::MAX_INFLUENCES; ++i)
		{
			SkinWeight.InfluenceBones[i] = FbxWeight.BoneIndices[i];
			SkinWeight.InfluenceWeights[i] = FbxWeight.BoneWeights[i];
		}

		OutSkinWeights.Add(SkinWeight);
	}

	UE_LOG("[FbxManager] 스킨 가중치 변환 완료: %d 정점", OutSkinWeights.Num());
}

void FFbxManager::ConvertFbxSkeletalToStaticMesh(const FFbxSkeletalMeshInfo& FbxData, FStaticMesh* OutStaticMesh)
{
	if (!OutStaticMesh)
	{
		UE_LOG_ERROR("유효하지 않은 StaticMesh입니다.");
		return;
	}

	// Vertices 변환
	for (int i = 0; i < FbxData.VertexList.Num(); ++i)
	{
		FNormalVertex Vertex{};
		Vertex.Position = FbxData.VertexList[i];
		Vertex.Normal = FbxData.NormalList.IsValidIndex(i) ? FbxData.NormalList[i] : FVector(0, 1, 0);
		Vertex.TexCoord = FbxData.TexCoordList.IsValidIndex(i) ? FbxData.TexCoordList[i] : FVector2(0, 0);
		Vertex.Tangent = FbxData.TangentList.IsValidIndex(i) ? FbxData.TangentList[i] : FVector4(1, 0, 0, 1);
		OutStaticMesh->Vertices.Add(Vertex);
	}

	OutStaticMesh->Indices = FbxData.Indices;

	// Materials 변환
	for (const FFbxMaterialInfo& FbxMat : FbxData.Materials)
	{
		FMaterial Material{};
		Material.Name = FbxMat.MaterialName;
		Material.Kd = FVector(0.9f, 0.9f, 0.9f);
		Material.Ka = FVector(0.2f, 0.2f, 0.2f);
		Material.Ks = FVector(0.5f, 0.5f, 0.5f);
		Material.Ns = 32.0f;
		Material.D = 1.0f;

		if (!FbxMat.DiffuseTexturePath.empty())
		{
			Material.KdMap = FbxMat.DiffuseTexturePath.generic_string();
		}
		if (!FbxMat.NormalTexturePath.empty())
		{
			Material.NormalMap = FbxMat.NormalTexturePath.generic_string();
		}
		OutStaticMesh->MaterialInfo.Add(Material);
	}

	// Sections 변환
	for (const FFbxMeshSection& FbxSection : FbxData.Sections)
	{
		FMeshSection Section{};
		Section.StartIndex = FbxSection.StartIndex;
		Section.IndexCount = FbxSection.IndexCount;
		Section.MaterialSlot = FbxSection.MaterialIndex;
		OutStaticMesh->Sections.Add(Section);
	}

	UE_LOG("[FbxManager] StaticMesh 변환 완료 - Vertices: %d, Indices: %d, Sections: %d",
		OutStaticMesh->Vertices.Num(), OutStaticMesh->Indices.Num(), OutStaticMesh->Sections.Num());
}

FStaticMesh* FFbxManager::GetOrCreateStaticMeshFromInfo(const FName& FilePath, const FFbxSkeletalMeshInfo& MeshInfo)
{
	// 캐시에서 먼저 찾기
	auto* FoundValuePtr = FbxSkeletalFStaticMeshMap.Find(FilePath);
	if (FoundValuePtr)
	{
		UE_LOG("[FbxManager] StaticMesh 캐시 히트: %s", FilePath.ToString().c_str());
		return FoundValuePtr->get();
	}

	// 캐시에 없으면 이미 로드된 MeshInfo로부터 FStaticMesh 생성
	auto StaticMeshAsset = std::make_unique<FStaticMesh>();
	ConvertFbxSkeletalToStaticMesh(MeshInfo, StaticMeshAsset.get());
	StaticMeshAsset->PathFileName = FilePath;

	UE_LOG_SUCCESS("[FbxManager] FBX SkeletalMesh Asset 변환 완료 (중복 로드 방지): %s", FilePath.ToString().c_str());

	// 캐시에 저장
	FbxSkeletalFStaticMeshMap.Emplace(FilePath, std::move(StaticMeshAsset));
	return FbxSkeletalFStaticMeshMap[FilePath].get();
}

// ========================================
// 🔸 Memory Management
// ========================================

void FFbxManager::Release()
{
	// unique_ptr이므로 Empty()만으로 자동 메모리 해제
	FbxFStaticMeshMap.Empty();
	FbxSkeletalFStaticMeshMap.Empty();
}
