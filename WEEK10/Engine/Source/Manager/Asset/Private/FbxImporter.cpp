#include "pch.h"
#include "Source/Manager/Asset/Public/FbxImporter.h"
#include "Source/Manager/Path/Public/PathManager.h"
#include "Core/Public/WindowsBinReader.h"
#include "Core/Public/WindowsBinWriter.h"

// ========================================
// 🔸 Public API
// ========================================

struct VertexKey
{
	FVector Position;
	FVector Normal;
	FVector2 UV;
	FVector4 Tangent; // W=Handedness

	bool operator==(const VertexKey& Other) const
	{
		return Position == Other.Position &&
			Normal == Other.Normal &&
			UV == Other.UV &&
			Tangent == Other.Tangent;
	}
};

struct VertexKeyHasher
{
	size_t operator()(const VertexKey& V) const
	{
		auto h1 = std::hash<float>()(V.Position.X) ^
			(std::hash<float>()(V.Position.Y) << 1) ^
			(std::hash<float>()(V.Position.Z) << 2);

		auto h2 = std::hash<float>()(V.Normal.X) ^
			(std::hash<float>()(V.Normal.Y) << 1) ^
			(std::hash<float>()(V.Normal.Z) << 2);

		auto h3 = std::hash<float>()(V.UV.X) ^
			(std::hash<float>()(V.UV.Y) << 1);

		auto h4 = std::hash<float>()(V.Tangent.X) ^
			(std::hash<float>()(V.Tangent.Y) << 1) ^
			(std::hash<float>()(V.Tangent.Z) << 2) ^
			(std::hash<float>()(V.Tangent.W) << 3);

		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
	}
};

bool FFbxImporter::Initialize()
{
	if (SdkManager) { return true; }

	SdkManager = FbxManager::Create();
	IoSettings = FbxIOSettings::Create(SdkManager, IOSROOT);
	SdkManager->SetIOSettings(IoSettings);

	UE_LOG_SUCCESS("FBX SDK Initialized.");
	return true;
}

void FFbxImporter::Shutdown()
{
	if (!SdkManager) { return; }

	IoSettings->Destroy();
	SdkManager->Destroy();
	SdkManager = nullptr;
	IoSettings = nullptr;

	UE_LOG_SUCCESS("FBX SDK Shut down.");
}

EFbxMeshType FFbxImporter::DetermineMeshType(const std::filesystem::path& FilePath)
{
	if (!SdkManager)
	{
		UE_LOG_ERROR("FBX SDK Manager가 초기화되지 않았습니다.");
		return EFbxMeshType::Unknown;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("FBX 파일이 존재하지 않습니다: %s", FilePath.string().c_str());
		return EFbxMeshType::Unknown;
	}

	// FBX Scene 임포트
	FbxScene* Scene = ImportFbxScene(FilePath, false);
	if (!Scene) { return EFbxMeshType::Unknown; }

	// Scene RAII 보장
	FFbxSceneGuard SceneGuard(Scene);

	FbxNode* RootNode = Scene->GetRootNode();
	if (!RootNode) { return EFbxMeshType::Unknown; }

	// 첫 번째 스킨 메시가 있는지 확인
	FbxNode* SkinnedMeshNode = nullptr;
	FbxMesh* SkinnedMesh = FindFirstSkinnedMesh(RootNode, &SkinnedMeshNode);

	if (HasAnySkinnedMesh(RootNode)) { return EFbxMeshType::Skeletal; }

	// 일반 메시가 있는지 확인
	FbxNode* MeshNode = nullptr;
	if (FbxMesh* Mesh = FindFirstMesh(RootNode, &MeshNode)) { return EFbxMeshType::Static; }

	return EFbxMeshType::Unknown;
}

bool FFbxImporter::LoadStaticMesh(const std::filesystem::path& FilePath, FFbxStaticMeshInfo* OutMeshInfo, Configuration Config)
{
	if (!OutMeshInfo)
	{
		UE_LOG_ERROR("유효하지 않은 FBXStaticMeshInfo입니다.");
		return false;
	}

	// 캐시 체크
	path CookedPath = UPathManager::GetInstance().GetCookedPath();
	path BinFilePath = CookedPath / (FilePath.stem().wstring() + L".fbxbin");
	if (Config.bIsBinaryEnabled && std::filesystem::exists(BinFilePath))
	{
		auto FbxTime = std::filesystem::last_write_time(FilePath);
		auto BinTime = std::filesystem::last_write_time(BinFilePath);

		if (BinTime >= FbxTime)
		{
			FWindowsBinReader Reader(BinFilePath);
			Reader << *OutMeshInfo;
			return true;
		}
	}

	if (!SdkManager || !std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("FBX SDK Manager 또는 파일이 유효하지 않습니다.");
		return false;
	}

	// FBX Scene 임포트 (원본 좌표계가 달랐는지 확인)
	bool bReverseWinding = false;
	FbxScene* Scene = ImportFbxScene(FilePath, true, &bReverseWinding);
	if (!Scene) { return false; }

	// Scene RAII 보장
	FFbxSceneGuard SceneGuard(Scene);

	FbxNode* RootNode = Scene->GetRootNode();
	if (!RootNode) { return false; }

	FbxNode* MeshNode = nullptr;
	FbxMesh* Mesh = FindFirstMesh(RootNode, &MeshNode);
	if (!Mesh || !MeshNode) { return false; }

	FbxGeometryConverter Converter(SdkManager);
	if (!Mesh->IsTriangleMesh())
	{
		FbxNodeAttribute* TriAttr = Converter.Triangulate(Mesh, true);
		if (TriAttr && TriAttr->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			Mesh = MeshNode->GetMesh(); // 새로운 메시 재할당
		}
	}

	ExtractVertices(Mesh, OutMeshInfo, Config);
	ExtractMaterials(MeshNode, FilePath, OutMeshInfo);
	ExtractGeometryData(Mesh, OutMeshInfo, Config, bReverseWinding);

	if (Config.bIsBinaryEnabled)
	{
		FWindowsBinWriter Writer(BinFilePath);
		Writer << *OutMeshInfo;
	}

	return true;
}

// ========================================
// 🔸 Private Helper Functions
// ========================================

FbxScene* FFbxImporter::ImportFbxScene(const std::filesystem::path& FilePath, bool bTriangulateScene, bool* OutNeedsWindingReversal)
{
	FbxImporter* Importer = FbxImporter::Create(SdkManager, "");
	if (!Importer->Initialize(FilePath.string().c_str(), -1, IoSettings))
	{
		UE_LOG_ERROR("FBX 파일을 로드 실패했습니다: %s", FilePath.string().c_str());
		Importer->Destroy();
		return nullptr;
	}

	FbxScene* Scene = FbxScene::Create(SdkManager, "Scene");
	Importer->Import(Scene);
	Importer->Destroy();

	// FBX 파일의 원본 좌표계 확인 (변환 전에 체크)
	FbxAxisSystem SceneAxisSystem = Scene->GetGlobalSettings().GetAxisSystem();

	int upSign, frontSign;
	FbxAxisSystem::EUpVector UpVector = SceneAxisSystem.GetUpVector(upSign);
	FbxAxisSystem::EFrontVector FrontVector = SceneAxisSystem.GetFrontVector(frontSign);
	FbxAxisSystem::ECoordSystem CoordSystem = SceneAxisSystem.GetCoorSystem();

	UE_LOG("[FbxImporter] FBX 원본 좌표계:");
	UE_LOG("  - Up: %s (sign: %d), Front: %s (sign: %d), %s",
		UpVector == FbxAxisSystem::eXAxis ? "X" :
		UpVector == FbxAxisSystem::eYAxis ? "Y" : "Z", upSign,
		FrontVector == FbxAxisSystem::eParityEven ? "ParityEven" : "ParityOdd", frontSign,
		CoordSystem == FbxAxisSystem::eRightHanded ? "RightHanded" : "LeftHanded");

	// 엔진 좌표계로 자동 변환 (Z-up, X-forward, Left-handed - 언리얼 스타일)
	FbxAxisSystem EngineAxisSystem(
		FbxAxisSystem::eZAxis,        // Z-up
		FbxAxisSystem::eParityOdd,    // X-forward (ParityOdd + ZAxis = X forward)
		FbxAxisSystem::eLeftHanded    // Left-handed
	);

	// 변환 전에 winding order 반전 필요 여부 판단
	bool bNeedsReversal = false;
	if (SceneAxisSystem != EngineAxisSystem)
	{
		bNeedsReversal = true;
		UE_LOG("[FbxImporter] 엔진 좌표계로 변환 중... (Z-up, X-forward, LH)");
		EngineAxisSystem.ConvertScene(Scene);
	}
	else
	{
		UE_LOG("[FbxImporter] 이미 엔진 좌표계와 일치합니다.");
	}

	// 호출자에게 winding order 반전 필요 여부 전달
	if (OutNeedsWindingReversal)
	{
		*OutNeedsWindingReversal = bNeedsReversal;
	}

	// 모든 지오메트리를 삼각형으로 변환
	if (bTriangulateScene)
	{
		FbxGeometryConverter GeomConverter(SdkManager);
		GeomConverter.Triangulate(Scene, true);
	}

	return Scene;
}

FbxMesh* FFbxImporter::FindFirstMesh(FbxNode* RootNode, FbxNode** OutNode)
{
	for (int i = 0; i < RootNode->GetChildCount(); ++i)
	{
		FbxNode* Child = RootNode->GetChild(i);
		if (FbxMesh* Mesh = Child->GetMesh())
		{
			*OutNode = Child;
			return Mesh;
		}
	}
	return nullptr;
}

void FFbxImporter::ExtractVertices(FbxMesh* Mesh, FFbxStaticMeshInfo* OutMeshInfo, const Configuration& Config)
{
	const int ControlPointCount = Mesh->GetControlPointsCount();
	FbxVector4* ControlPoints = Mesh->GetControlPoints();

	OutMeshInfo->VertexList.Reserve(ControlPointCount);
	for (int i = 0; i < ControlPointCount; ++i)
	{
		// ConvertScene()이 이미 엔진 좌표계로 변환했으므로 그대로 사용
		FVector Pos(ControlPoints[i][0], ControlPoints[i][1], ControlPoints[i][2]);
		OutMeshInfo->VertexList.Add(Pos);
	}
}

void FFbxImporter::ExtractMaterials(FbxNode* Node, const std::filesystem::path& FbxFilePath, FFbxStaticMeshInfo* OutMeshInfo, uint32 MaterialOffset)
{
	const int MaterialCount = Node->GetMaterialCount();

	for (int m = 0; m < MaterialCount; ++m)
	{
		FbxSurfaceMaterial* Material = Node->GetMaterial(m);
		if (!Material) { continue; }

		FFbxMaterialInfo MatInfo;
		const char* MaterialName = Material->GetName();
		MatInfo.MaterialName = (MaterialName && strlen(MaterialName) > 0)
			? MaterialName
			: "Material_" + std::to_string(MaterialOffset + m);

		// 🔸 Diffuse 텍스처 추출
		if (FbxProperty Prop = Material->FindProperty(FbxSurfaceMaterial::sDiffuse); Prop.IsValid())
		{
			int TextureCount = Prop.GetSrcObjectCount<FbxFileTexture>();
			if (TextureCount > 0)
			{
				if (FbxFileTexture* Texture = Prop.GetSrcObject<FbxFileTexture>(0))
				{
					std::string OriginalTexturePath = Texture->GetFileName();
					std::filesystem::path ResolvedPath = ResolveTexturePath(
						OriginalTexturePath, FbxFilePath.parent_path(), FbxFilePath);
					if (!ResolvedPath.empty())
						MatInfo.DiffuseTexturePath = ResolvedPath;
				}
			}
		}

		// 🔸 Normal 맵 추출 (Bump 포함)
		if (FbxProperty NormalProp = Material->FindProperty(FbxSurfaceMaterial::sNormalMap); NormalProp.IsValid())
		{
			int TextureCount = NormalProp.GetSrcObjectCount<FbxFileTexture>();
			if (TextureCount > 0)
			{
				if (FbxFileTexture* Texture = NormalProp.GetSrcObject<FbxFileTexture>(0))
				{
					std::string OriginalTexturePath = Texture->GetFileName();
					std::filesystem::path ResolvedPath = ResolveTexturePath(
						OriginalTexturePath, FbxFilePath.parent_path(), FbxFilePath);
					if (!ResolvedPath.empty())
						MatInfo.NormalTexturePath = ResolvedPath;
				}
			}
		}
		else if (FbxProperty BumpProp = Material->FindProperty(FbxSurfaceMaterial::sBump); BumpProp.IsValid())
		{
			int TextureCount = BumpProp.GetSrcObjectCount<FbxFileTexture>();
			if (TextureCount > 0)
			{
				if (FbxFileTexture* Texture = BumpProp.GetSrcObject<FbxFileTexture>(0))
				{
					std::string OriginalTexturePath = Texture->GetFileName();
					std::filesystem::path ResolvedPath = ResolveTexturePath(
						OriginalTexturePath, FbxFilePath.parent_path(), FbxFilePath);
					if (!ResolvedPath.empty())
					{
						MatInfo.NormalTexturePath = ResolvedPath;
						UE_LOG("[FbxImporter] Bump 맵을 Normal 맵으로 사용합니다.");
					}
				}
			}
		}

		// 🔹 중복 검사: 같은 텍스처 경로를 이미 가진 Material은 스킵
		bool bDuplicate = false;
		for (const auto& Existing : OutMeshInfo->Materials)
		{
			if (Existing.DiffuseTexturePath == MatInfo.DiffuseTexturePath &&
				Existing.NormalTexturePath == MatInfo.NormalTexturePath)
			{
				bDuplicate = true;
				break;
			}
			// 이름이 같은 경우도 중복으로 간주
			if (Existing.MaterialName == MatInfo.MaterialName)
			{
				bDuplicate = true;
				break;
			}
		}

		if (!bDuplicate)
		{
			OutMeshInfo->Materials.Add(MatInfo);
			UE_LOG("[FbxImporter] Material 추가: %s", MatInfo.MaterialName.c_str());
		}
		else
		{
			UE_LOG_WARNING("[FbxImporter] 중복 Material 무시됨: %s", MatInfo.MaterialName.c_str());
		}
	}

	// 🔸 Material이 하나도 없으면 기본 Material 추가
	if (MaterialCount == 0 && MaterialOffset == 0 && OutMeshInfo->Materials.Num() == 0)
	{
		FFbxMaterialInfo DefaultMat;
		DefaultMat.MaterialName = "Default";
		OutMeshInfo->Materials.Add(DefaultMat);
		UE_LOG_WARNING("[FbxImporter] 기본 Material 추가됨 (No materials found).");
	}
}


std::filesystem::path FFbxImporter::ResolveTexturePath(
	const std::string& OriginalPath,
	const std::filesystem::path& FbxDirectory,
	const std::filesystem::path& FbxFilePath)
{
	std::filesystem::path OriginalFsPath(OriginalPath);

	// 방법 1: 원본 경로가 유효한지 확인
	if (std::filesystem::exists(OriginalFsPath))
	{
		UE_LOG_SUCCESS("[FbxImporter] 텍스처 찾음 (원본 경로): %s", OriginalFsPath.string().c_str());
		return OriginalFsPath;
	}

	// 방법 2: FBX 파일과 같은 디렉토리에서 파일명만으로 찾기
	std::filesystem::path FilenameOnly = OriginalFsPath.filename();
	std::filesystem::path LocalTexturePath = FbxDirectory / FilenameOnly;

	if (std::filesystem::exists(LocalTexturePath))
	{
		UE_LOG_SUCCESS("[FbxImporter] 텍스처 찾음 (FBX 디렉토리): %s", LocalTexturePath.string().c_str());
		return LocalTexturePath;
	}

	// 방법 3: .fbm 폴더에서 찾기 (FBX SDK 기본 텍스처 저장 위치)
	std::filesystem::path FbxFilename = FbxFilePath.stem();
	std::filesystem::path FbmFolder = FbxDirectory / (FbxFilename.string() + ".fbm");
	std::filesystem::path FbmTexturePath = FbmFolder / FilenameOnly;

	if (std::filesystem::exists(FbmTexturePath))
	{
		UE_LOG_SUCCESS("[FbxImporter] 텍스처 찾음 (.fbm 폴더): %s", FbmTexturePath.string().c_str());
		return FbmTexturePath;
	}

	UE_LOG_WARNING("[FbxImporter] 텍스처를 찾을 수 없습니다: %s", OriginalPath.c_str());
	UE_LOG_WARNING("[FbxImporter] 시도한 경로: %s", FbmTexturePath.string().c_str());
	return {};
}

void FFbxImporter::ExtractGeometryData(FbxMesh* Mesh, FFbxStaticMeshInfo* OutMeshInfo, const Configuration& Config, bool bReverseWinding)
{
	FbxLayerElementMaterial* MaterialElement = Mesh->GetElementMaterial();
	FbxGeometryElement::EMappingMode MaterialMode =
		MaterialElement ? MaterialElement->GetMappingMode() : FbxGeometryElement::eNone;

	// 기존 ControlPoint 기반 Position 백업
	TArray<FVector> ControlPointPositions = OutMeshInfo->VertexList;

	// 기존 데이터 비우기
	OutMeshInfo->VertexList.Empty();
	OutMeshInfo->NormalList.Empty();
	OutMeshInfo->TexCoordList.Empty();
	OutMeshInfo->TangentList.Empty();
	OutMeshInfo->Indices.Empty();

	// Material 별 인덱스 리스트
	TArray<TArray<uint32>> IndicesPerMaterial;
	IndicesPerMaterial.SetNum(OutMeshInfo->Materials.Num());

	// ⭐ Dedup 테이블
	std::unordered_map<VertexKey, uint32, VertexKeyHasher> VertexCache;

	const int PolygonCount = Mesh->GetPolygonCount();

	const FbxGeometryElementNormal* LayerNormal = Mesh->GetElementNormal(0);
	const FbxGeometryElementTangent* LayerTangent = Mesh->GetElementTangent(0);

	// Tangent 없으면 자동 생성
	if (!LayerTangent)
	{
		Mesh->GenerateTangentsDataForAllUVSets();
		LayerTangent = Mesh->GetElementTangent(0);
	}

	FbxStringList UVSetNames;
	Mesh->GetUVSetNames(UVSetNames);

	// 폴리곤 순회
	for (int p = 0; p < PolygonCount; ++p)
	{
		// Material index
		int MatIndex = 0;
		if (MaterialElement)
		{
			if (MaterialMode == FbxGeometryElement::eByPolygon)
				MatIndex = MaterialElement->GetIndexArray().GetAt(p);
			else
				MatIndex = 0;
		}
		if (!IndicesPerMaterial.IsValidIndex(MatIndex))
			MatIndex = 0;

		// 폴리곤은 Triangulate 되어 항상 3
		for (int v = 0; v < 3; ++v)
		{
			int CPIndex = Mesh->GetPolygonVertex(p, v);

			// -------- Position
			FVector Position = (CPIndex >= 0 && CPIndex < ControlPointPositions.Num())
				? ControlPointPositions[CPIndex]
				: FVector(0, 0, 0);

			// -------- Normal
			FbxVector4 N;
			Mesh->GetPolygonVertexNormal(p, v, N);
			FVector Normal(N[0], N[1], N[2]);

			// -------- Tangent
			FbxVector4 T(1, 0, 0, 1);
			int PolyVertIndex = p * 3 + v;

			if (LayerTangent)
			{
				if (LayerTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					if (LayerTangent->GetReferenceMode() == FbxGeometryElement::eDirect)
					{
						T = LayerTangent->GetDirectArray().GetAt(PolyVertIndex);
					}
					else
					{
						int TIdx = LayerTangent->GetIndexArray().GetAt(PolyVertIndex);
						T = LayerTangent->GetDirectArray().GetAt(TIdx);
					}
				}
			}

			FVector Tangent(T[0], T[1], T[2]);
			FVector BiTangent = Normal.Cross(Tangent);
			float Handedness = (BiTangent.Length() > 0.0001f) ? 1.0f : -1.0f;

			FVector4 Tangent4(Tangent.X, Tangent.Y, Tangent.Z, Handedness);

			// -------- UV
			FVector2 Tex(0, 0);
			if (Mesh->GetElementUVCount() > 0)
			{
				const FbxGeometryElementUV* ElemUV = Mesh->GetElementUV(0);
				int UVIndex = Mesh->GetTextureUVIndex(p, v);

				FbxVector2 UV = ElemUV->GetDirectArray().GetAt(UVIndex);
				Tex = FVector2(UV[0], 1.0f - UV[1]);
			}

			// Key 생성
			VertexKey Key;
			Key.Position = Position;
			Key.Normal = Normal;
			Key.UV = Tex;
			Key.Tangent = Tangent4;

			uint32 FinalIndex;
			auto Found = VertexCache.find(Key);

			if (Found != VertexCache.end())
			{
				// 이미 있는 정점 → 인덱스 재사용
				FinalIndex = Found->second;
			}
			else
			{
				// 새 정점
				FinalIndex = OutMeshInfo->VertexList.Num();
				VertexCache.emplace(Key, FinalIndex);

				OutMeshInfo->VertexList.Add(Position);
				OutMeshInfo->NormalList.Add(Normal);
				OutMeshInfo->TexCoordList.Add(Tex);
				OutMeshInfo->TangentList.Add(Tangent4);
			}

			// 인덱스 추가
			IndicesPerMaterial[MatIndex].Add(FinalIndex);
		}
	}

	// Material 정렬로 최종 인덱스 조립
	OutMeshInfo->Indices.Empty();
	for (int i = 0; i < IndicesPerMaterial.Num(); ++i)
	{
		if (!bReverseWinding)
		{
			for (uint32 idx : IndicesPerMaterial[i])
				OutMeshInfo->Indices.Add(idx);
		}
		else
		{
			for (int j = 0; j < IndicesPerMaterial[i].Num(); j += 3)
			{
				if (j + 2 < IndicesPerMaterial[i].Num())
				{
					OutMeshInfo->Indices.Add(IndicesPerMaterial[i][j + 2]);
					OutMeshInfo->Indices.Add(IndicesPerMaterial[i][j + 1]);
					OutMeshInfo->Indices.Add(IndicesPerMaterial[i][j + 0]);
				}
			}
		}
	}

	// Section 생성
	BuildMeshSections(IndicesPerMaterial, OutMeshInfo);
}

void FFbxImporter::BuildMeshSections(const TArray<TArray<uint32>>& IndicesPerMaterial, FFbxStaticMeshInfo* OutMeshInfo)
{
	uint32 CurrentIndexOffset = 0;
	for (int i = 0; i < IndicesPerMaterial.Num(); ++i)
	{
		if (IndicesPerMaterial[i].Num() > 0)
		{
			FFbxMeshSection Section;
			Section.StartIndex = CurrentIndexOffset;
			Section.IndexCount = IndicesPerMaterial[i].Num();
			Section.MaterialIndex = i;
			OutMeshInfo->Sections.Add(Section);

			UE_LOG("[FbxImporter] Section %d: StartIndex=%d, Count=%d, MaterialIndex=%d",
				i, Section.StartIndex, Section.IndexCount, Section.MaterialIndex);

			CurrentIndexOffset += Section.IndexCount;
		}
	}
}

bool FFbxImporter::HasAnySkinnedMesh(FbxNode* Root)
{
	if (!Root) { return false; }

	if (FbxMesh* Mesh = Root->GetMesh())
	{
		if (Mesh->GetDeformerCount(FbxDeformer::eSkin) > 0) { return true; }
	}

	for (int i = 0; i < Root->GetChildCount(); ++i)
	{
		if (HasAnySkinnedMesh(Root->GetChild(i))) { return true; }
	}

	return false;
}

bool FFbxImporter::EnsureTriangleMesh(FbxMesh*& Mesh, FbxGeometryConverter& Converter)
{
	if (!Mesh) { return false; }
	if (Mesh->IsTriangleMesh()) { return true; }

	FbxNode* Node = Mesh->GetNode();
	if (!Node) { return false; }

	// Mesh 단위로 Triangulate 진행
	FbxNodeAttribute* TriAttr = Converter.Triangulate(Mesh, true);
	if (TriAttr && TriAttr->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		Mesh = Node->GetMesh(); // 교체된 메시 참조로 갱신
		return true;
	}
	return false;
}

// ========================================
// 🔸 Skeletal Mesh Implementation
// ========================================

bool FFbxImporter::LoadSkeletalMesh(const std::filesystem::path& FilePath, FFbxSkeletalMeshInfo* OutMeshInfo, Configuration Config)
{
	// 입력 검증
	if (!OutMeshInfo)
	{
		UE_LOG_ERROR("유효하지 않은 FFbxSkeletalMeshInfo입니다.");
		return false;
	}

	// .fbxbin 파일의 여부 확인
	path CookedPath = UPathManager::GetInstance().GetCookedPath();
	path BinFilePath = CookedPath / (FilePath.stem().wstring() + L".fbxbin");
	if (Config.bIsBinaryEnabled && std::filesystem::exists(BinFilePath))
	{
		auto FbxTime = std::filesystem::last_write_time(FilePath);
		auto BinTime = std::filesystem::last_write_time(BinFilePath);

		if (BinTime >= FbxTime)
		{
			UE_LOG_SUCCESS("FbxCache: Loaded cached fbxbin '%ls'", BinFilePath.c_str());
			FWindowsBinReader WindowsBinReader(BinFilePath);
			WindowsBinReader << *OutMeshInfo;
			return true;
		}
		else
		{
			UE_LOG_INFO("FbxCache: fbxbin outdated, reloading from fbx '%ls'", FilePath.c_str());
		}
	}

	if (!SdkManager)
	{
		UE_LOG_ERROR("FBX SDK Manager가 존재하지 않습니다.");
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("FBX 파일이 존재하지 않습니다: %s", FilePath.string().c_str());
		return false;
	}

	OutMeshInfo->PathFileName = FName(FilePath.string());

	// FBX Scene 임포트
	FbxScene* Scene = ImportFbxScene(FilePath);
	if (!Scene) { return false; }

	// Scene RAII 보장
	FFbxSceneGuard SceneGuard(Scene);

	// 모든 스킨 메시 찾기 (디버그)
	FbxNode* RootNode = Scene->GetRootNode();
	if (!RootNode)
	{
		UE_LOG_ERROR("FBX 루트 노드를 찾을 수 없습니다.");
		return false;
	}

	// 디버그: 모든 메시 노드 출력
	int TotalMeshCount = 0;
	int SkinnedMeshCount = 0;
	std::function<void(FbxNode*)> CountMeshes = [&](FbxNode* Node) {
		if (FbxMesh* Mesh = Node->GetMesh())
		{
			TotalMeshCount++;
			int DeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
			if (DeformerCount > 0)
			{
				SkinnedMeshCount++;
				UE_LOG("[FbxImporter] 스킨 메시 발견 #%d: '%s' (정점: %d, 폴리곤: %d)",
					SkinnedMeshCount, Node->GetName(),
					Mesh->GetControlPointsCount(), Mesh->GetPolygonCount());
			}
		}
		for (int i = 0; i < Node->GetChildCount(); ++i)
		{
			CountMeshes(Node->GetChild(i));
		}
	};
	CountMeshes(RootNode);
	UE_LOG("[FbxImporter] 전체 메시: %d개, 스킨 메시: %d개", TotalMeshCount, SkinnedMeshCount);

	FbxNode* MeshNode = nullptr;
	FbxMesh* Mesh = FindFirstSkinnedMesh(RootNode, &MeshNode);
	if (!Mesh || !MeshNode)
	{
		UE_LOG_ERROR("FBX에 유효한 스켈레탈 메시가 없습니다");
		return false;
	}

	// 모든 스킨 메시 찾기
	TArray<FbxNode*> AllMeshNodes;
	FindAllSkinnedMeshes(RootNode, AllMeshNodes);

	if (AllMeshNodes.Num() == 0)
	{
		UE_LOG_ERROR("FBX에 유효한 스킨 메시가 없습니다");
		return false;
	}

	UE_LOG_SUCCESS("[FbxImporter] 총 %d개의 스킨 메시를 병합합니다", AllMeshNodes.Num());

	// 첫 번째 메시에서 스켈레톤 추출
	FbxMesh* FirstMesh = AllMeshNodes[0]->GetMesh();
	if (!ExtractSkeleton(Scene, FirstMesh, OutMeshInfo))
	{
		UE_LOG_ERROR("스켈레톤 추출 실패");
		return false;
	}

	// 모든 메시 병합
	for (int i = 0; i < AllMeshNodes.Num(); ++i)
	{
		FbxNode* CurrentNode = AllMeshNodes[i];
		FbxMesh* CurrentMesh = CurrentNode->GetMesh();

		UE_LOG("[FbxImporter] 메시 #%d 처리 중: '%s'", i + 1, CurrentNode->GetName());

		uint32 VertexOffset = OutMeshInfo->VertexList.Num();
		uint32 MaterialOffset = OutMeshInfo->Materials.Num();

		// ControlPoint 오프셋 계산
		int32 ControlPointOffset = 0;
		if (OutMeshInfo->ControlPointIndices.Num() > 0)
		{
			for (int32 Idx : OutMeshInfo->ControlPointIndices)
			{
				if (Idx > ControlPointOffset)
				{
					ControlPointOffset = Idx;
				}
			}
			ControlPointOffset++; // 다음 인덱스부터 시작
		}

		// 머티리얼 먼저 추가 (통합된 ExtractMaterials 사용)
		ExtractMaterials(CurrentNode, FilePath, OutMeshInfo, MaterialOffset);

		// 지오메트리 데이터 추가 (통합된 함수 사용)
		ExtractSkeletalGeometryData(CurrentMesh, OutMeshInfo, Config, VertexOffset, MaterialOffset, ControlPointOffset);

		// 스킨 가중치 추가 (통합된 함수 사용)
		if (!ExtractSkinWeights(CurrentMesh, OutMeshInfo, VertexOffset, ControlPointOffset))
		{
			UE_LOG_ERROR("스킨 가중치 처리 실패: 메시 #%d", i + 1);
			return false;
		}
	}

	// 파싱 완료 후 베이크 저장
	if (Config.bIsBinaryEnabled)
	{
		FWindowsBinWriter WindowsBinWriter(BinFilePath);
		WindowsBinWriter << *OutMeshInfo;
		UE_LOG_SUCCESS("FbxCache: Saved fbxbin '%ls'", BinFilePath.c_str());
	}

	UE_LOG_SUCCESS("스켈레탈 메시 로드 완료: %s", FilePath.string().c_str());
	return true;
}

FbxMesh* FFbxImporter::FindFirstSkinnedMesh(FbxNode* RootNode, FbxNode** OutNode)
{
	for (int i = 0; i < RootNode->GetChildCount(); ++i)
	{
		FbxNode* Child = RootNode->GetChild(i);
		if (FbxMesh* Mesh = Child->GetMesh())
		{
			// 스킨 디포머가 있는지 확인
			int DeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
			if (DeformerCount > 0)
			{
				*OutNode = Child;
				return Mesh;
			}
		}

		// 재귀적으로 자식 노드 탐색
		if (FbxMesh* FoundMesh = FindFirstSkinnedMesh(Child, OutNode))
		{
			return FoundMesh;
		}
	}
	return nullptr;
}

void FFbxImporter::FindAllSkinnedMeshes(FbxNode* RootNode, TArray<FbxNode*>& OutMeshNodes)
{
	if (!RootNode) { return; }

	// 현재 노드에 메시가 있는지 확인
	if (FbxMesh* Mesh = RootNode->GetMesh())
	{
		int DeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);

		// 폴리곤이 존재하는 메시라면 추가
		if (DeformerCount > 0 || Mesh->GetPolygonCount() > 0)
		{
			OutMeshNodes.Add(RootNode);
			if (DeformerCount == 0)
			{
				UE_LOG_WARNING("[FbxImporter] 스킨이 없는 메시 발견: '%s' (예: 눈동자, 치아 등)", RootNode->GetName());
			}
		}
	}

	// 자식 노드 재귀 탐색
	for (int i = 0; i < RootNode->GetChildCount(); ++i)
	{
		FindAllSkinnedMeshes(RootNode->GetChild(i), OutMeshNodes);
	}
}

bool FFbxImporter::ExtractSkeleton(FbxScene* Scene, FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo)
{
	// 스킨 디포머 찾기
	FbxSkin* Skin = nullptr;
	int DeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
	if (DeformerCount == 0)
	{
		UE_LOG_ERROR("메시에 스킨 디포머가 없습니다.");
		return false;
	}

	Skin = (FbxSkin*)Mesh->GetDeformer(0, FbxDeformer::eSkin);
	if (!Skin)
	{
		UE_LOG_ERROR("스킨 디포머를 가져올 수 없습니다.");
		return false;
	}

	int ClusterCount = Skin->GetClusterCount();
	if (ClusterCount == 0)
	{
		UE_LOG_ERROR("본 클러스터가 없습니다.");
		return false;
	}

	UE_LOG("[FbxImporter] 본 개수: %d", ClusterCount);

	// 본 정보를 임시로 저장할 맵 (FbxNode* -> BoneIndex)
	TMap<FbxNode*, int32> BoneNodeToIndexMap;

	// 1차: 모든 본 수집
	for (int i = 0; i < ClusterCount; ++i)
	{
		FbxCluster* Cluster = Skin->GetCluster(i);
		FbxNode* LinkNode = Cluster->GetLink();
		if (!LinkNode)
			continue;

		if (BoneNodeToIndexMap.Find(LinkNode))
			continue; // 이미 추가된 본

		FFbxBoneInfo BoneInfo;
		BoneInfo.BoneName = LinkNode->GetName();
		BoneInfo.ParentIndex = -1; // 나중에 설정

		// 로컬 변환 추출 (ConvertScene()이 이미 엔진 좌표계로 변환했으므로 그대로 사용)
		FbxAMatrix LocalTransform = LinkNode->EvaluateLocalTransform();
		FbxVector4 T = LocalTransform.GetT();
		FbxQuaternion R = LocalTransform.GetQ();
		FbxVector4 S = LocalTransform.GetS();

		BoneInfo.LocalTransform.Translation = FVector(T[0], T[1], T[2]);
		BoneInfo.LocalTransform.Rotation = FQuaternion(R[0], R[1], R[2], R[3]);
		BoneInfo.LocalTransform.Scale = FVector(S[0], S[1], S[2]);

		int32 BoneIndex = OutMeshInfo->Bones.Num();
		OutMeshInfo->Bones.Add(BoneInfo);
		BoneNodeToIndexMap.Add(LinkNode, BoneIndex);

		UE_LOG("[FbxImporter] 본 %d: %s", BoneIndex, BoneInfo.BoneName.c_str());
	}

	// 2차: 부모 관계 설정
	for (int32 i = 0; i < OutMeshInfo->Bones.Num(); ++i)
	{
		// 본 노드 찾기
		FbxNode* BoneNode = nullptr;
		for (auto& Pair : BoneNodeToIndexMap)
		{
			if (Pair.second == i)
			{
				BoneNode = Pair.first;
				break;
			}
		}

		if (!BoneNode)
			continue;

		FbxNode* ParentNode = BoneNode->GetParent();
		if (ParentNode)
		{
			int32* ParentIndexPtr = BoneNodeToIndexMap.Find(ParentNode);
			if (ParentIndexPtr)
			{
				OutMeshInfo->Bones[i].ParentIndex = *ParentIndexPtr;
			}
		}
	}

	return true;
}

bool FFbxImporter::ExtractSkinWeights(FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo, uint32 VertexOffset, int32 ControlPointOffset)
{
	// 스킨이 없는 메시일 경우 (예: 눈동자)
	if (Mesh->GetDeformerCount(FbxDeformer::eSkin) == 0)
	{
		UE_LOG_WARNING("[FbxImporter] 스킨이 없는 메시입니다. (예: 눈동자) - 기본 Influence로 채웁니다.");

		// 현재 버텍스 개수 계산
		int32 CurrentVertexCount = OutMeshInfo->VertexList.Num();
		for (uint32 i = VertexOffset; i < CurrentVertexCount; ++i)
		{
			OutMeshInfo->SkinWeights.Add(FFbxBoneInfluence());
		}

		return true; // 스킨 없음 처리 완료
	}

	// 기존 스킨 처리 루틴 (변경 없음)
	FbxSkin* Skin = (FbxSkin*)Mesh->GetDeformer(0, FbxDeformer::eSkin);
	if (!Skin)
	{
		UE_LOG_ERROR("스킨 디포머를 찾을 수 없습니다.");
		return false;
	}

	const int ControlPointCount = Mesh->GetControlPointsCount();
	int ClusterCount = Skin->GetClusterCount();

	// 1단계: ControlPoint 기반으로 가중치 추출
	TArray<FFbxBoneInfluence> ControlPointWeights;
	ControlPointWeights.Reset(ControlPointCount);
	ControlPointWeights.SetNum(ControlPointCount);

	for (int ClusterIndex = 0; ClusterIndex < ClusterCount; ++ClusterIndex)
	{
		FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
		int* Indices = Cluster->GetControlPointIndices();
		double* Weights = Cluster->GetControlPointWeights();
		int IndexCount = Cluster->GetControlPointIndicesCount();

		for (int i = 0; i < IndexCount; ++i)
		{
			int CtrlPointIndex = Indices[i];
			double Weight = Weights[i];

			if (CtrlPointIndex >= 0 && CtrlPointIndex < ControlPointCount && Weight > 0.0001)
			{
				FFbxBoneInfluence& Influence = ControlPointWeights[CtrlPointIndex];

				for (int j = 0; j < FFbxBoneInfluence::MAX_INFLUENCES; ++j)
				{
					if (Influence.BoneIndices[j] == -1)
					{
						Influence.BoneIndices[j] = ClusterIndex;
						Influence.BoneWeights[j] = static_cast<uint8>(Weight * 255.0);
						break;
					}
				}
			}
		}
	}

	// 2단계: ControlPoint → Vertex 매핑
	int32 CurrentVertexCount = OutMeshInfo->VertexList.Num();
	if (VertexOffset == 0)
	{
		OutMeshInfo->SkinWeights.Reset(CurrentVertexCount);
	}

	for (uint32 i = VertexOffset; i < CurrentVertexCount; ++i)
	{
		int32 CtrlPointIndex = OutMeshInfo->ControlPointIndices[i];
		int32 LocalCtrlPointIndex = CtrlPointIndex - ControlPointOffset;

		if (LocalCtrlPointIndex >= 0 && LocalCtrlPointIndex < ControlPointCount)
		{
			if (VertexOffset == 0)
				OutMeshInfo->SkinWeights[i] = ControlPointWeights[LocalCtrlPointIndex];
			else
				OutMeshInfo->SkinWeights.Add(ControlPointWeights[LocalCtrlPointIndex]);
		}
		else
		{
			if (VertexOffset == 0)
				OutMeshInfo->SkinWeights[i] = FFbxBoneInfluence();
			else
				OutMeshInfo->SkinWeights.Add(FFbxBoneInfluence());
		}
	}

	UE_LOG_SUCCESS("[FbxImporter] 스킨 가중치 %s: ControlPoints=%d, Vertices=%d",
		VertexOffset == 0 ? "추출 완료" : "추가 완료",
		ControlPointCount, CurrentVertexCount - VertexOffset);
	return true;
}

void FFbxImporter::ExtractSkeletalGeometryData(FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo, const Configuration& Config,
	uint32 VertexOffset, uint32 MaterialOffset, int32 ControlPointOffset)
{
	// 컨트롤 포인트(정점) 추출
	const int ControlPointCount = Mesh->GetControlPointsCount();
	FbxVector4* ControlPoints = Mesh->GetControlPoints();

	TArray<FVector> ControlPointPositions;
	ControlPointPositions.Reserve(ControlPointCount);

	for (int i = 0; i < ControlPointCount; ++i)
	{
		// ConvertScene()이 이미 엔진 좌표계로 변환했으므로 그대로 사용
		FVector Pos(ControlPoints[i][0], ControlPoints[i][1], ControlPoints[i][2]);
		ControlPointPositions.Add(Pos);
	}

	// Material Mapping
	FbxLayerElementMaterial* MaterialElement = Mesh->GetElementMaterial();
	FbxGeometryElement::EMappingMode MaterialMappingMode = FbxGeometryElement::eNone;
	if (MaterialElement)
	{
		MaterialMappingMode = MaterialElement->GetMappingMode();
	}

	// Material별 인덱스 그룹 (첫 메시인 경우와 추가 메시인 경우 처리 방식이 다름)
	TArray<TArray<uint32>> IndicesPerMaterial;
	if (VertexOffset == 0)
	{
		// 첫 메시: 기존 로직
		IndicesPerMaterial.Reset(OutMeshInfo->Materials.Num() > 0 ? OutMeshInfo->Materials.Num() : 1);
		for (int i = 0; i < (OutMeshInfo->Materials.Num() > 0 ? OutMeshInfo->Materials.Num() : 1); ++i)
		{
			IndicesPerMaterial.Add(TArray<uint32>());
		}
	}
	else
	{
		// 추가 메시: 전체 Material 개수만큼 설정
		IndicesPerMaterial.SetNum(OutMeshInfo->Materials.Num());
	}

	// Dedup 테이블
	std::unordered_map<VertexKey, uint32, VertexKeyHasher> VertexCache;

	const FbxGeometryElementTangent* LayerTangent = Mesh->GetElementTangent(0);

	// Tangent가 없으면 자동 생성
	if (!LayerTangent)
	{
		UE_LOG("[FbxImporter] Tangent 데이터가 없어 자동 생성합니다.");
		bool bResult = Mesh->GenerateTangentsDataForAllUVSets();
		if (bResult)
		{
			LayerTangent = Mesh->GetElementTangent(0);
			UE_LOG_SUCCESS("[FbxImporter] Tangent 자동 생성 완료");
		}
		else
		{
			UE_LOG_WARNING("[FbxImporter] Tangent 자동 생성 실패");
		}
	}

	// 폴리곤별 처리
	const int PolygonCount = Mesh->GetPolygonCount();
	for (int p = 0; p < PolygonCount; ++p)
	{
		int MaterialIndex = 0;
		if (MaterialElement)
		{
			switch (MaterialMappingMode)
			{
			case FbxGeometryElement::eByPolygon:
				MaterialIndex = MaterialElement->GetIndexArray().GetAt(p);
				break;
			case FbxGeometryElement::eAllSame:
				MaterialIndex = 0;
				break;
			}
		}

		// Material 오프셋 적용
		MaterialIndex += MaterialOffset;
		if (MaterialIndex < 0 || MaterialIndex >= IndicesPerMaterial.Num())
		{
			MaterialIndex = (MaterialOffset > 0) ? OutMeshInfo->Materials.Num() - 1 : 0;
		}

		int PolySize = Mesh->GetPolygonSize(p);
		for (int v = 0; v < PolySize; ++v)
		{
			int CtrlPointIndex = Mesh->GetPolygonVertex(p, v);

			// -------- Position
			FVector Position = (CtrlPointIndex >= 0 && CtrlPointIndex < ControlPointPositions.Num())
				? ControlPointPositions[CtrlPointIndex]
				: FVector(0, 0, 0);

			// -------- Normal
			FbxVector4 N;
			Mesh->GetPolygonVertexNormal(p, v, N);
			FVector Normal(N[0], N[1], N[2]);

			// -------- Tangent
			FbxVector4 T(1, 0, 0, 1);
			int PolyVertIndex = p * 3 + v;

			if (LayerTangent)
			{
				if (LayerTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					if (LayerTangent->GetReferenceMode() == FbxGeometryElement::eDirect)
					{
						T = LayerTangent->GetDirectArray().GetAt(PolyVertIndex);
					}
					else
					{
						int TIdx = LayerTangent->GetIndexArray().GetAt(PolyVertIndex);
						T = LayerTangent->GetDirectArray().GetAt(TIdx);
					}
				}
			}

			FVector Tangent(T[0], T[1], T[2]);
			FVector BiTangent = Normal.Cross(Tangent);
			float Handedness = (BiTangent.Length() > 0.0001f) ? 1.0f : -1.0f;

			FVector4 Tangent4(Tangent.X, Tangent.Y, Tangent.Z, Handedness);

			// -------- UV
			FVector2 Tex(0, 0);
			if (Mesh->GetElementUVCount() > 0)
			{
				const FbxGeometryElementUV* ElemUV = Mesh->GetElementUV(0);
				int UVIndex = Mesh->GetTextureUVIndex(p, v);

				FbxVector2 UV = ElemUV->GetDirectArray().GetAt(UVIndex);
				Tex = FVector2(UV[0], 1.0f - UV[1]);
			}

			// -------- Dedup Key 생성
			VertexKey Key;
			Key.Position = Position;
			Key.Normal = Normal;
			Key.UV = Tex;
			Key.Tangent = Tangent4;

			uint32 FinalIndex;
			auto Found = VertexCache.find(Key);

			if (Found != VertexCache.end())
			{
				// 이미 있는 정점 → 인덱스 재사용
				FinalIndex = Found->second;
			}
			else
			{
				// 새 정점
				FinalIndex = OutMeshInfo->VertexList.Num();
				VertexCache.emplace(Key, FinalIndex);

				OutMeshInfo->VertexList.Add(Position);
				OutMeshInfo->NormalList.Add(Normal);
				OutMeshInfo->TexCoordList.Add(Tex);
				OutMeshInfo->TangentList.Add(Tangent4);
				OutMeshInfo->ControlPointIndices.Add(CtrlPointIndex + ControlPointOffset);
			}

			// 인덱스 추가
			IndicesPerMaterial[MaterialIndex].Add(FinalIndex);
		}
	}

	// Indices를 Material별로 재정렬 (첫 메시만)
	if (VertexOffset == 0)
	{
		OutMeshInfo->Indices.Empty();
		for (int i = 0; i < IndicesPerMaterial.Num(); ++i)
		{
			// Right-handed → Left-handed 변환시 Winding Order 뒤집기
			// 삼각형 단위로 (0,1,2) → (2,1,0)으로 역순 변환
			for (int j = 0; j < IndicesPerMaterial[i].Num(); j += 3)
			{
				if (j + 2 < IndicesPerMaterial[i].Num())
				{
					OutMeshInfo->Indices.Add(IndicesPerMaterial[i][j + 2]);  // 2
					OutMeshInfo->Indices.Add(IndicesPerMaterial[i][j + 1]);  // 1
					OutMeshInfo->Indices.Add(IndicesPerMaterial[i][j + 0]);  // 0
				}
			}
		}
		BuildMeshSections(IndicesPerMaterial, OutMeshInfo);
	}
	else
	{
		// 추가 메시: Section만 업데이트
		uint32 CurrentOffset = 0;
		if (OutMeshInfo->Sections.Num() > 0)
		{
			const auto& Last = OutMeshInfo->Sections.Last();
			CurrentOffset = Last.StartIndex + Last.IndexCount;
		}

		for (int i = 0; i < IndicesPerMaterial.Num(); ++i)
		{
			if (IndicesPerMaterial[i].Num() == 0) continue;

			// Right-handed → Left-handed 변환시 Winding Order 뒤집기
			for (int j = 0; j < IndicesPerMaterial[i].Num(); j += 3)
			{
				if (j + 2 < IndicesPerMaterial[i].Num())
				{
					OutMeshInfo->Indices.Add(IndicesPerMaterial[i][j + 2]);  // 2
					OutMeshInfo->Indices.Add(IndicesPerMaterial[i][j + 1]);  // 1
					OutMeshInfo->Indices.Add(IndicesPerMaterial[i][j + 0]);  // 0
				}
			}

			FFbxMeshSection Section;
			Section.StartIndex = CurrentOffset;
			Section.IndexCount = IndicesPerMaterial[i].Num();
			Section.MaterialIndex = i;
			OutMeshInfo->Sections.Add(Section);
			CurrentOffset += Section.IndexCount;
		}
	}
}
