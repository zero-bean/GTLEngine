#include "pch.h"
#include "Manager/Asset/Public/AssetManager.h"

#include "Render/Renderer/Public/Renderer.h"
#include "DirectXTK/WICTextureLoader.h"
#include "DirectXTK/DDSTextureLoader.h"
#include "Component/Mesh/Public/VertexDatas.h"
#include "Manager/Asset/Public/LODMaker.h"
#include "Physics/Public/AABB.h"
#include "Texture/Public/TextureRenderProxy.h"
#include "Texture/Public/Texture.h"
#include "Manager/Asset/Public/ObjManager.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UAssetManager)

UAssetManager::UAssetManager() = default;

UAssetManager::~UAssetManager() = default;

void UAssetManager::Initialize()
{
	URenderer& Renderer = URenderer::GetInstance();

	// Data 폴더 속 모든 .obj 파일 로드 및 캐싱
	LoadAllObjStaticMesh();

	// TMap.Add()
	VertexDatas.emplace(EPrimitiveType::Cube, &VerticesCube);
	VertexDatas.emplace(EPrimitiveType::Sphere, &VerticesSphere);
	VertexDatas.emplace(EPrimitiveType::Triangle, &VerticesTriangle);
	VertexDatas.emplace(EPrimitiveType::Square, &VerticesSquare);
	VertexDatas.emplace(EPrimitiveType::Torus, &VerticesTorus);
	VertexDatas.emplace(EPrimitiveType::Arrow, &VerticesArrow);
	VertexDatas.emplace(EPrimitiveType::CubeArrow, &VerticesCubeArrow);
	VertexDatas.emplace(EPrimitiveType::Ring, &VerticesRing);
	VertexDatas.emplace(EPrimitiveType::Line, &VerticesLine);
	VertexDatas.emplace(EPrimitiveType::Decal, &VerticesCube);
	// jft
	VertexDatas.emplace(EPrimitiveType::Spotlight, &VerticesCube);

	IndexDatas.emplace(EPrimitiveType::Cube, &IndicesCube);
	IndexDatas.emplace(EPrimitiveType::Decal, &IndicesCubeLine);
	IndexDatas.emplace(EPrimitiveType::Spotlight, &IndicesCubeLine);

	IndexBuffers.emplace(EPrimitiveType::Cube,
		Renderer.CreateIndexBuffer(IndicesCube.data(), static_cast<int>(IndicesCube.size()) * sizeof(uint32)));
	IndexBuffers.emplace(EPrimitiveType::Decal,
		Renderer.CreateIndexBuffer(IndicesCubeLine.data(), static_cast<int>(IndicesCubeLine.size()) * sizeof(uint32)));
	IndexBuffers.emplace(EPrimitiveType::Spotlight,
		Renderer.CreateIndexBuffer(IndicesCubeLine.data(), static_cast<int>(IndicesCubeLine.size()) * sizeof(uint32)));

	NumIndices.emplace(EPrimitiveType::Cube, static_cast<uint32>(IndicesCube.size()));
	NumIndices.emplace(EPrimitiveType::Decal, static_cast<uint32>(IndicesCubeLine.size()));
	NumIndices.emplace(EPrimitiveType::Spotlight, static_cast<uint32>(IndicesCubeLine.size()));

	// TArray.GetData(), TArray.Num()*sizeof(FVertexSimple), TArray.GetTypeSize()
	VertexBuffers.emplace(EPrimitiveType::Cube, Renderer.CreateVertexBuffer(
		VerticesCube.data(), static_cast<int>(VerticesCube.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::Sphere, Renderer.CreateVertexBuffer(
		VerticesSphere.data(), static_cast<int>(VerticesSphere.size() * sizeof(FNormalVertex))));
	VertexBuffers.emplace(EPrimitiveType::Triangle, Renderer.CreateVertexBuffer(
		VerticesTriangle.data(), static_cast<int>(VerticesTriangle.size() * sizeof(FNormalVertex))));
	VertexBuffers.emplace(EPrimitiveType::Square, Renderer.CreateVertexBuffer(
		VerticesSquare.data(), static_cast<int>(VerticesSquare.size() * sizeof(FNormalVertex))));
	VertexBuffers.emplace(EPrimitiveType::Torus, Renderer.CreateVertexBuffer(
		VerticesTorus.data(), static_cast<int>(VerticesTorus.size() * sizeof(FNormalVertex))));
	VertexBuffers.emplace(EPrimitiveType::Arrow, Renderer.CreateVertexBuffer(
		VerticesArrow.data(), static_cast<int>(VerticesArrow.size() * sizeof(FNormalVertex))));
	VertexBuffers.emplace(EPrimitiveType::CubeArrow, Renderer.CreateVertexBuffer(
		VerticesCubeArrow.data(), static_cast<int>(VerticesCubeArrow.size() * sizeof(FNormalVertex))));
	VertexBuffers.emplace(EPrimitiveType::Ring, Renderer.CreateVertexBuffer(
		VerticesRing.data(), static_cast<int>(VerticesRing.size() * sizeof(FNormalVertex))));
	VertexBuffers.emplace(EPrimitiveType::Line, Renderer.CreateVertexBuffer(
		VerticesLine.data(), static_cast<int>(VerticesLine.size() * sizeof(FNormalVertex))));
	VertexBuffers.emplace(EPrimitiveType::Decal, VertexBuffers[EPrimitiveType::Cube]);
	VertexBuffers.emplace(EPrimitiveType::Spotlight, VertexBuffers[EPrimitiveType::Cube]);

	NumVertices.emplace(EPrimitiveType::Cube, static_cast<uint32>(VerticesCube.size()));
	NumVertices.emplace(EPrimitiveType::Sphere, static_cast<uint32>(VerticesSphere.size()));
	NumVertices.emplace(EPrimitiveType::Triangle, static_cast<uint32>(VerticesTriangle.size()));
	NumVertices.emplace(EPrimitiveType::Square, static_cast<uint32>(VerticesSquare.size()));
	NumVertices.emplace(EPrimitiveType::Torus, static_cast<uint32>(VerticesTorus.size()));
	NumVertices.emplace(EPrimitiveType::Arrow, static_cast<uint32>(VerticesArrow.size()));
	NumVertices.emplace(EPrimitiveType::CubeArrow, static_cast<uint32>(VerticesCubeArrow.size()));
	NumVertices.emplace(EPrimitiveType::Ring, static_cast<uint32>(VerticesRing.size()));
	NumVertices.emplace(EPrimitiveType::Line, static_cast<uint32>(VerticesLine.size()));
	NumVertices.emplace(EPrimitiveType::Decal, NumVertices[EPrimitiveType::Cube]);
	NumVertices.emplace(EPrimitiveType::Spotlight, NumVertices[EPrimitiveType::Cube]);

	// Calculate AABB for all primitive types (excluding StaticMesh)
	for (const auto& Pair : VertexDatas)
	{
		EPrimitiveType Type = Pair.first;
		const auto* Vertices = Pair.second;
		if (!Vertices || Vertices->empty())
			continue;

		AABBs[Type] = CalculateAABB(*Vertices);
	}

	// Calculate AABB for each StaticMesh
	for (const auto& MeshPair : StaticMeshCache)
	{
		const FName& ObjPath = MeshPair.first;
		const auto& Mesh = MeshPair.second;
		if (!Mesh || !Mesh->IsValid())
			continue;

		const auto& Vertices = Mesh->GetVertices();
		if (Vertices.empty())
			continue;

		StaticMeshAABBs[ObjPath] = CalculateAABB(Vertices);
	}

	// Initialize Shaders
	ID3D11VertexShader* vertexShader;
	ID3D11InputLayout* inputLayout;
	TArray<D3D11_INPUT_ELEMENT_DESC> layoutDesc =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	URenderer::GetInstance().CreateVertexShaderAndInputLayout(L"Asset/Shader/BatchLineVS.hlsl", layoutDesc,
		&vertexShader, &inputLayout);
	VertexShaders.emplace(EShaderType::BatchLine, vertexShader);
	InputLayouts.emplace(EShaderType::BatchLine, inputLayout);

	ID3D11PixelShader* PixelShader;
	URenderer::GetInstance().CreatePixelShader(L"Asset/Shader/BatchLinePS.hlsl", &PixelShader);
	PixelShaders.emplace(EShaderType::BatchLine, PixelShader);
}

void UAssetManager::Release()
{
	// Texture Resource 해제
	ReleaseAllTextures();

	// Material Resource 해제
	ReleaseAllMaterials();

	// TMap.Value()
	for (auto& Pair : VertexBuffers)
	{
		URenderer::GetInstance().ReleaseVertexBuffer(Pair.second);
	}
	for (auto& Pair : IndexBuffers)
	{
		URenderer::GetInstance().ReleaseIndexBuffer(Pair.second);
	}

	for (auto& Pair : StaticMeshVertexBuffers)
	{
		URenderer::GetInstance().ReleaseVertexBuffer(Pair.second);
	}
	for (auto& Pair : StaticMeshIndexBuffers)
	{
		URenderer::GetInstance().ReleaseIndexBuffer(Pair.second);
	}

	StaticMeshCache.clear();	// unique ptr 이라서 자동으로 해제됨
	StaticMeshVertexBuffers.clear();
	StaticMeshIndexBuffers.clear();

	// TMap.Empty()
	VertexBuffers.clear();
	IndexBuffers.clear();

	BillboardSpriteOptions.clear();
	DecalTextureOptions.clear();
}

/**
 * @brief Data/ 경로 하위에 모든 .obj 파일을 로드 후 캐싱한다
 */
void UAssetManager::LoadAllObjStaticMesh()
{
	URenderer& Renderer = URenderer::GetInstance();

	TArray<FName> ObjList, LODObjList;
	const FString DataDirectory = "Data/"; // 검색할 기본 디렉토리

	auto IsInLodFolder = [](const std::filesystem::path& P) -> bool
	{
		for (std::filesystem::path Cur = P; !Cur.empty(); Cur = Cur.parent_path())
		{
			std::string Name = Cur.filename().string();
			std::transform(Name.begin(), Name.end(), Name.begin(),
				[](unsigned char C){ return static_cast<char>(std::tolower(C)); });

			if (Name == "lod")
			{
				return true;
			}
		}
		return false;
	};

	if (std::filesystem::exists(DataDirectory) && std::filesystem::is_directory(DataDirectory))
	{
		std::filesystem::recursive_directory_iterator It(
			DataDirectory, std::filesystem::directory_options::skip_permission_denied
		), End;

		for (; It != End; ++It)
		{
			const auto& Entry = *It;

			if (Entry.is_regular_file() && Entry.path().extension() == ".obj")
			{
				const bool bInLOD = IsInLodFolder(Entry.path().parent_path());
				FString PathString = Entry.path().generic_string();

				if (bInLOD)
				{
					LODObjList.push_back(FName(PathString));
				}
				else
				{
					ObjList.push_back(FName(PathString));
				}
			}
		}
	}

	// Enable winding order flip for this OBJ file
	FObjImporter::Configuration Config;
	Config.bFlipWindingOrder = false;
	Config.bIsBinaryEnabled = true;
	Config.bUVToUEBasis = true;
	Config.bPositionToUEBasis = true;

	TArray<float> ReductionRatios = { 0.5f, 0.25f };

	for (const FName& ObjPath : ObjList)
	{
		//for (float ReductionRatio : ReductionRatios)
		//{
		//	FMeshSimplifier MeshSimplifier;
		//	if (MeshSimplifier.LoadFromObj(ObjPath.ToString()))
		//	{
		//		MeshSimplifier.Simplify(ReductionRatio);
		//		if (MeshSimplifier.SaveToObj(ObjPath.ToString(), ReductionRatio))
		//		{
		//			// LOD 파일 경로 구성 (SaveToObj와 동일한 로직)
		//			std::filesystem::path OriginalPath(ObjPath.ToString());
		//			std::filesystem::path LodDirectory = OriginalPath.parent_path() / "LOD";

		//			// 파일명 구성 (예: A_lod_050.obj, A_lod_025.obj)
		//			FString OriginalStem = OriginalPath.stem().string();
		//			int RatioInt = static_cast<int>(ReductionRatio * 100);
		//			FString RatioStr = (RatioInt < 100) ? "0" + std::to_string(RatioInt) : std::to_string(RatioInt);
		//			if (RatioInt < 10) RatioStr = "0" + RatioStr;

		//			std::filesystem::path LodObjPath = LodDirectory / (OriginalStem + "_lod_" + RatioStr + ".obj");

		//			// 경로를 정규화하여 / 또는 \ 차이로 인한 중복 방지
		//			FString NormalizedPath = LodObjPath.generic_string();
		//			FName LodObjName(NormalizedPath);

		//			// 중복 체크 후 LODObjList에 추가 (정규화된 경로로 비교)
		//			bool bAlreadyExists = false;
		//			for (const FName& ExistingPath : LODObjList)
		//			{
		//				std::filesystem::path ExistingNormalized(ExistingPath.ToString());
		//				FString ExistingGeneric = ExistingNormalized.generic_string();

		//				if (NormalizedPath == ExistingGeneric)
		//				{
		//					bAlreadyExists = true;
		//					break;
		//				}
		//			}

		//			if (!bAlreadyExists)
		//			{
		//				LODObjList.push_back(LodObjName);
		//			}
		//		}
		//	}
		//}

		if (UStaticMesh* LoadedMesh = FObjManager::LoadObjStaticMesh(ObjPath, Config))
		{
			StaticMeshCache.emplace(ObjPath, LoadedMesh);
			StaticMeshVertexBuffers.emplace(ObjPath, CreateVertexBuffer(LoadedMesh->GetVertices()));
			StaticMeshIndexBuffers.emplace(ObjPath, CreateIndexBuffer(LoadedMesh->GetIndices()));
		}
	}

	for (auto& Path : LODObjList)
	{
		UE_LOG("Path : %s", Path.ToString().data());
	}

	// LOD Mesh 로드 및 원본 메시와 연결
	for (const FName& LODObjPath : LODObjList)
	{
		if (UStaticMesh* LoadedLODMesh = FObjManager::LoadObjStaticMesh(LODObjPath, Config))
		{
			StaticMeshCache.emplace(LODObjPath, LoadedLODMesh);
			StaticMeshVertexBuffers.emplace(LODObjPath, CreateVertexBuffer(LoadedLODMesh->GetVertices()));
			StaticMeshIndexBuffers.emplace(LODObjPath, CreateIndexBuffer(LoadedLODMesh->GetIndices()));

			// LOD 파일에서 원본 파일 경로 추출하여 연결
			FString LODPathStr = LODObjPath.ToString();
			std::filesystem::path LODPath(LODPathStr);

			// LOD 파일명에서 원본 파일명 추출 (예: apple_lod_050.obj -> apple.obj)
			FString FileName = LODPath.stem().string();
			size_t LodPos = FileName.find("_lod_");
			if (LodPos != FString::npos)
			{
				FString OriginalStem = FileName.substr(0, LodPos);

				// 원본 파일 경로 구성 (LOD 폴더의 부모 디렉토리에서 찾기)
				std::filesystem::path OriginalPath = LODPath.parent_path().parent_path() / (OriginalStem + ".obj");

				// 경로 정규화: 백슬래시를 슬래시로 변경
				FString OriginalPathStr = OriginalPath.string();
				std::replace(OriginalPathStr.begin(), OriginalPathStr.end(), '\\', '/');

				FName OriginalObjPath(OriginalPathStr);

				// 원본 메시가 캐시에 있으면 LOD 연결
				auto OriginalMeshIter = StaticMeshCache.find(OriginalObjPath);
				if (OriginalMeshIter != StaticMeshCache.end())
				{
					UStaticMesh* OriginalMesh = OriginalMeshIter->second.get();
					if (OriginalMesh && LoadedLODMesh->GetStaticMeshAsset())
					{
						OriginalMesh->AddLODMesh(LoadedLODMesh->GetStaticMeshAsset());
					}
				}
			}
		}
	}
}

ID3D11Buffer* UAssetManager::GetVertexBuffer(FName InObjPath)
{
	if (StaticMeshVertexBuffers.count(InObjPath))
	{
		return StaticMeshVertexBuffers[InObjPath];
	}
	return nullptr;
}

ID3D11Buffer* UAssetManager::GetIndexBuffer(FName InObjPath)
{
	if (StaticMeshIndexBuffers.count(InObjPath))
	{
		return StaticMeshIndexBuffers[InObjPath];
	}
	return nullptr;
}

/**
 * @brief 머티리얼을 생성하고 캐시에 저장하는 함수
 * @param InMaterialName 생성할 머티리얼의 이름 (키로 사용)
 * @param InTexturePath 머티리얼에 사용할 텍스처 파일 경로 (옵션)
 * @return 성공 시 UMaterial 포인터, 실패 시 nullptr
 */
UMaterial* UAssetManager::CreateMaterial(const FName& InMaterialName, const FName& InTexturePath)
{
	if (HasMaterial(InMaterialName))
	{
		return GetMaterial(InMaterialName);
	}

	auto NewMaterial = std::make_unique<UMaterial>();
	if (!NewMaterial)
	{
		return nullptr;
	}

	// 텍스처 경로가 유효하면 텍스처를 생성하여 머티리얼에 설정
	if (InTexturePath != FName::GetNone() && !InTexturePath.ToString().empty())
	{
		UTexture* Texture = CreateTexture(InTexturePath);
		if (Texture)
		{
			NewMaterial->SetDiffuseTexture(Texture);
		}
		else
		{
			UE_LOG_WARNING("머티리얼 '%s'에 대한 텍스처 로드 실패: %s", InMaterialName.ToString().c_str(), InTexturePath.ToString().c_str());
		}
	}

	// 기본 셰이더 설정 (필요에 따라 수정)
	// NewMaterial->SetVertexShader(GetVertexShader(EShaderType::Default));
	// NewMaterial->SetPixelShader(GetPixelShader(EShaderType::Default));

	UMaterial* MaterialPtr = NewMaterial.get();
	MaterialCache.emplace(InMaterialName, std::move(NewMaterial));

	return MaterialPtr;
}

UMaterial* UAssetManager::GetMaterial(const FName& InMaterialName)
{
	if (auto Iter = MaterialCache.find(InMaterialName); Iter != MaterialCache.end())
	{
		return Iter->second.get();
	}

	return nullptr;
}

void UAssetManager::ReleaseMaterial(UMaterial* InMaterial)
{
	if (InMaterial == nullptr) { return; }

	for (auto Iter = MaterialCache.begin(); Iter != MaterialCache.end(); ++Iter)
	{
		if (Iter->second.get() == InMaterial)
		{
			MaterialCache.erase(Iter);
			return;
		}
	}
}

bool UAssetManager::HasMaterial(const FName& InMaterialName) const
{
	return MaterialCache.find(InMaterialName) != MaterialCache.end();
}

ID3D11Buffer* UAssetManager::CreateVertexBuffer(TArray<FNormalVertex> InVertices)
{
	return URenderer::GetInstance().CreateVertexBuffer(InVertices.data(), static_cast<int>(InVertices.size()) * sizeof(FNormalVertex));
}

ID3D11Buffer* UAssetManager::CreateIndexBuffer(TArray<uint32> InIndices)
{
	return URenderer::GetInstance().CreateIndexBuffer(InIndices.data(), static_cast<int>(InIndices.size()) * sizeof(uint32));
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

ID3D11Buffer* UAssetManager::GetIndexbuffer(EPrimitiveType InType)
{
	return IndexBuffers[InType];
}

uint32 UAssetManager::GetNumIndices(EPrimitiveType InType)
{
	return NumIndices[InType];
}

ID3D11VertexShader* UAssetManager::GetVertexShader(EShaderType Type)
{
	return VertexShaders[Type];
}

ID3D11PixelShader* UAssetManager::GetPixelShader(EShaderType Type)
{
	return PixelShaders[Type];
}

ID3D11InputLayout* UAssetManager::GetIputLayout(EShaderType Type)
{
	return InputLayouts[Type];
}

void UAssetManager::LoadAssets()
{
	// 빌보드 아이콘 로드
	const std::filesystem::path IconDirectory = std::filesystem::absolute(std::filesystem::path("Asset/Icon"));
	if (std::filesystem::exists(IconDirectory))
	{
		UAssetManager& AssetManager = UAssetManager::GetInstance();
		for (const auto& Entry : std::filesystem::directory_iterator(IconDirectory))
		{
			if (!Entry.is_regular_file() || Entry.path().extension() != ".png") continue;

			FString FilePath = Entry.path().generic_string();
			FString DisplayName = Entry.path().stem().string();

			if (UTexture* Texture = AssetManager.CreateTexture(FilePath, DisplayName))
			{
				BillboardSpriteOptions.push_back({ DisplayName, FilePath, TObjectPtr(Texture) });
			}
		}
		std::sort(BillboardSpriteOptions.begin(), BillboardSpriteOptions.end(),
			[](const FTextureOption& A, const FTextureOption& B) {
				return A.DisplayName < B.DisplayName;
			});
	}

	// 데칼 텍스처 로드
	const std::filesystem::path DecalTextureDirectory = std::filesystem::absolute(std::filesystem::path("Asset/Texture/"));
	if (std::filesystem::exists(DecalTextureDirectory))
	{
		UAssetManager& AssetManager = UAssetManager::GetInstance();
		for (const auto& Entry : std::filesystem::directory_iterator(DecalTextureDirectory))
		{
			if (Entry.is_regular_file())
			{
				FString Extension = Entry.path().extension().string();
				std::transform(Extension.begin(), Extension.end(), Extension.begin(),
					[](unsigned char InChar) { return static_cast<char>(std::tolower(InChar)); });

				if (Extension == ".png" || Extension == ".jpg" || Extension == ".jpeg" || Extension == ".dds" || Extension == ".tga")
				{
					FString FilePath = Entry.path().generic_string();
					FString DisplayName = Entry.path().stem().string();

					if (UTexture* Texture = AssetManager.CreateTexture(FilePath, DisplayName))
					{
						DecalTextureOptions.push_back({ DisplayName, FilePath, TObjectPtr(Texture) });
					}
				}
			}
		}
		std::sort(DecalTextureOptions.begin(), DecalTextureOptions.end(),
			[](const FTextureOption& A, const FTextureOption& B) {
				return A.DisplayName < B.DisplayName;
			});
	}
}

const FAABB& UAssetManager::GetAABB(EPrimitiveType InType)
{
	return AABBs[InType];
}

const FAABB& UAssetManager::GetStaticMeshAABB(FName InName)
{
	return StaticMeshAABBs[InName];
}

const TArray<FTextureOption>& UAssetManager::GetBillboardSpriteOptions()
{
	if (BillboardSpriteOptions.size() == 0)
	{
		LoadAssets();
	}
	return BillboardSpriteOptions;
}

const TArray<FTextureOption>& UAssetManager::GetDecalTextureOptions()
{
	if (DecalTextureOptions.size() == 0)
	{
		LoadAssets();
	}
	return DecalTextureOptions;
}

/**
 * @brief 파일에서 텍스처를 로드하고 캐시에 저장하는 함수
 * 중복 로딩을 방지하기 위해 이미 로드된 텍스처는 캐시에서 반환
 * @param InFilePath 로드할 텍스처 파일의 경로
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 */
ComPtr<ID3D11ShaderResourceView> UAssetManager::LoadTexture(const FName& InFilePath, const FName& InName)
{
	// 이미 로드된 텍스처가 있는지 확인
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		return Iter->second;
	}

	// 새로운 텍스처 로드
	ID3D11ShaderResourceView* TextureSRV = CreateTextureFromFile(InFilePath.ToString());
	if (TextureSRV)
	{
		TextureCache[InFilePath] = TextureSRV;
	}

	return TextureSRV;
}

UTexture* UAssetManager::CreateTexture(const FName& InFilePath, const FName& InName)
{
	auto SRV = LoadTexture(InFilePath);
	if (!SRV)	return nullptr;

	ID3D11SamplerState* Sampler = nullptr;
	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;       // UV가 범위를 벗어나면 클램프
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = URenderer::GetInstance().GetDevice()->CreateSamplerState(&SamplerDesc, &Sampler);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("CreateSamplerState failed (HRESULT: 0x%08lX)", hr);
		return nullptr;
	}

	auto* Proxy = new FTextureRenderProxy(SRV, Sampler);
	auto* Texture = new UTexture(InFilePath, InName);
	Texture->SetRenderProxy(Proxy);

	return Texture;
}

/**
 * @brief 캐시된 텍스처를 가져오는 함수
 * 이미 로드된 텍스처만 반환하고 새로 로드하지는 않음
 * @param InFilePath 가져올 텍스처 파일의 경로
 * @return 캐시에 있으면 ID3D11ShaderResourceView 포인터, 없으면 nullptr
 */
ComPtr<ID3D11ShaderResourceView> UAssetManager::GetTexture(const FName& InFilePath)
{
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		return Iter->second;
	}

	return nullptr;
}

/**
 * @brief 특정 텍스처를 캐시에서 해제하는 함수
 * DirectX 리소스를 해제하고 캐시에서 제거
 * @param InFilePath 해제할 텍스처 파일의 경로
 */
void UAssetManager::ReleaseTexture(const FName& InFilePath)
{
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		if (Iter->second)
		{
			Iter->second->Release();
		}

		TextureCache.erase(Iter);
	}
}

/**
 * @brief 특정 텍스처가 캐시에 있는지 확인하는 함수
 * @param InFilePath 확인할 텍스처 파일의 경로
 * @return 캐시에 있으면 true, 없으면 false
 */
bool UAssetManager::HasTexture(const FName& InFilePath) const
{
	return TextureCache.find(InFilePath) != TextureCache.end();
}

/**
 * @brief 모든 텍스처 리소스를 해제하는 함수
 * 캐시된 모든 텍스처의 DirectX 리소스를 해제하고 캐시를 비움
 */
void UAssetManager::ReleaseAllTextures()
{
	for (auto& Pair : TextureCache)
	{
		if (Pair.second)
		{
			Pair.second->Release();
		}
	}
	TextureCache.clear();
}

void UAssetManager::ReleaseAllMaterials()
{
	MaterialCache.clear();
}

/**
 * @brief 파일에서 DirectX 텍스처를 생성하는 내부 함수
 * DirectXTK의 WICTextureLoader를 사용
 * @param InFilePath 로드할 이미지 파일의 경로
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 */
ID3D11ShaderResourceView* UAssetManager::CreateTextureFromFile(const path& InFilePath)
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext)
	{
		UE_LOG_ERROR("ResourceManager: Texture 생성 실패 - Device 또는 DeviceContext가 null입니다");
		return nullptr;
	}

	// 파일 확장자에 따라 적절한 로더 선택
	FString FileExtension = InFilePath.extension().string();
	transform(FileExtension.begin(), FileExtension.end(), FileExtension.begin(), ::tolower);

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT ResultHandle;

	try
	{
		if (FileExtension == ".dds")
		{
			// DDS 파일은 DDSTextureLoader 사용
			ResultHandle = DirectX::CreateDDSTextureFromFile(
				Device,
				DeviceContext,
				InFilePath.c_str(),
				nullptr,
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: DDS 텍스처 로드 성공 - %ls", InFilePath.c_str());
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: DDS 텍스처 로드 실패 - %ls (HRESULT: 0x%08lX)",
					InFilePath.c_str(), ResultHandle);
			}
		}
		else
		{
			// 기타 포맷은 WICTextureLoader 사용 (PNG, JPG, BMP, TIFF 등)
			ResultHandle = DirectX::CreateWICTextureFromFile(
				Device,
				DeviceContext,
				InFilePath.c_str(),
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: WIC 텍스처 로드 성공 - %ls", InFilePath.c_str());
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: WIC 텍스처 로드 실패 - %ls (HRESULT: 0x%08lX)"
					, InFilePath.c_str(), ResultHandle);
			}
		}
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("ResourceManager: 텍스처 로드 중 예외 발생 - %ls: %s", InFilePath.c_str(), Exception.what());
		return nullptr;
	}

	return SUCCEEDED(ResultHandle) ? TextureSRV : nullptr;
}

/**
 * @brief 메모리 데이터에서 DirectX 텍스처를 생성하는 함수
 * DirectXTK의 WICTextureLoader와 DDSTextureLoader를 사용하여 메모리 데이터에서 텍스처 생성
 * @param InData 이미지 데이터의 포인터
 * @param InDataSize 데이터의 크기 (Byte)
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 * @note DDS 포맷 감지를 위해 매직 넘버를 확인하고 적절한 로더 선택
 * @note 네트워크에서 다운로드한 이미지나 리소스 팩에서 추출한 데이터 처리에 유용
 */
ID3D11ShaderResourceView* UAssetManager::CreateTextureFromMemory(const void* InData, size_t InDataSize)
{
	if (!InData || InDataSize == 0)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 실패 - 잘못된 데이터");
		return nullptr;
	}

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 실패 - Device 또는 DeviceContext가 null입니다");
		return nullptr;
	}

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT ResultHandle;

	try
	{
		// DDS 매직 넘버 확인 (DDS 파일은 "DDS " 로 시작)
		const uint32 DDS_MAGIC = 0x20534444; // "DDS " in little-endian
		bool bIsDDS = (InDataSize >= 4 && *static_cast<const uint32*>(InData) == DDS_MAGIC);

		if (bIsDDS)
		{
			// DDS 데이터는 DDSTextureLoader 사용
			ResultHandle = DirectX::CreateDDSTextureFromMemory(
				Device,
				DeviceContext,
				static_cast<const uint8*>(InData),
				InDataSize,
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: DDS 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: DDS 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
			}
		}
		else
		{
			// 기타 포맷은 WICTextureLoader 사용 (PNG, JPG, BMP, TIFF 등)
			ResultHandle = DirectX::CreateWICTextureFromMemory(
				Device,
				DeviceContext,
				static_cast<const uint8*>(InData),
				InDataSize,
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: WIC 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: WIC 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
			}
		}
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 중 예외 발생: %s", Exception.what());
		return nullptr;
	}

	return SUCCEEDED(ResultHandle) ? TextureSRV : nullptr;
}

/**
 * @brief Vertex 배열로부터 AABB(Axis-Aligned Bounding Box)를 계산하는 헬퍼 함수
 * @param vertices 정점 데이터 배열
 * @return 계산된 FAABB 객체
 */
FAABB UAssetManager::CalculateAABB(const TArray<FNormalVertex>& Vertices)
{
	__m128 minv = _mm_set1_ps(FLT_MAX);
	__m128 maxv = _mm_set1_ps(-FLT_MAX);

	for (const auto& Vertex : Vertices)
	{
		__m128 p = _mm_setr_ps(Vertex.Position.X, Vertex.Position.Y, Vertex.Position.Z, 0.0f);
		minv = _mm_min_ps(minv, p);
		maxv = _mm_max_ps(maxv, p);
	}

	alignas(16) float tmpMin[4], tmpMax[4];
	_mm_store_ps(tmpMin, minv);
	_mm_store_ps(tmpMax, maxv);

	return FAABB(FVector(tmpMin[0], tmpMin[1], tmpMin[2]),
				 FVector(tmpMax[0], tmpMax[1], tmpMax[2]));
}
