#include "pch.h"
#include "MeshLoader.h"
#include "ObjectFactory.h"
#include "d3dtk/DDSTextureLoader.h"
#include "ObjManager.h"
#include "d3dtk/WICTextureLoader.h"
#include "TextQuad.h"

#define GRIDNUM 100
#define AXISLENGTH 100

UResourceManager::~UResourceManager()
{
    Clear();
}

UResourceManager& UResourceManager::GetInstance()
{
    static UResourceManager* Instance = nullptr;
    if (Instance == nullptr)
    {
        Instance = NewObject<UResourceManager>();
    }
    return *Instance;
}

void UResourceManager::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InContext)
{
    Device = InDevice;
    Resources.SetNum(static_cast<uint8>(ResourceType::End));

    Context = InContext;
    //CreateGridMesh(GRIDNUM,"Grid");
    //CreateAxisMesh(AXISLENGTH,"Axis");

    InitShaderILMap();


    CreateTextBillboardMesh();//"TextBillboard"
    CreateBillboardMesh();//"Billboard"

    CreateTextBillboardTexture();
    CreateScreenQuatMesh();
}

// 전체 해제
void UResourceManager::Clear()
{
    {////////////// Deprecated //////////////
        for (auto& [Key, Data] : ResourceMap)
        {
            if (Data)
            {
                if (Data->VertexBuffer)
                {
                    Data->VertexBuffer->Release();
                    Data->VertexBuffer = nullptr;
                }
                if (Data->IndexBuffer)
                {
                    Data->IndexBuffer->Release();
                    Data->IndexBuffer = nullptr;
                }
                delete Data;
            }
        }
        ResourceMap.clear();


        // TextureMap 해제
        for (auto& [Key, Data] : TextureMap)
        {
            if (Data)
            {
                if (Data->Texture) { Data->Texture->Release(); Data->Texture = nullptr; }
                if (Data->TextureSRV) { Data->TextureSRV->Release(); Data->TextureSRV = nullptr; }
                if (Data->BlendState) { Data->BlendState->Release(); Data->BlendState = nullptr; }
                delete Data;
            }
        }
        TextureMap.clear();

        // MaterialMap 해제
        for (auto& [Key, Mat] : MaterialMap)
        {
            if (Mat)
            {
                ObjectFactory::DeleteObject(Mat);
            }
        }
        MaterialMap.clear();
    }

    for (auto& Array : Resources)
    {
        for (auto& Resource : Array)
        {
            if(Resource.second)
            {
                DeleteObject(Resource.second);
                Resource.second = nullptr;
            }
        }
        Array.Empty();
    }
    Resources.Empty();

    // Instance lifetime is managed by ObjectFactory
}

void UResourceManager::CreateAxisMesh(float Length, const FString& FilePath)
{
    // 이미 있으면 패스
    if (ResourceMap[FilePath])
    {
        return;
    }

    TArray<FVector> axisVertices;
    TArray<FVector4> axisColors;
    TArray<uint32> axisIndices;

    // X축 (빨강)
    axisVertices.push_back(FVector(0.0f, 0.0f, 0.0f));       // 원점
    axisVertices.push_back(FVector(Length, 0.0f, 0.0f));     // +X
    axisColors.push_back(FVector4(1.0f, 0.0f, 0.0f, 1.0f));  // 빨강
    axisColors.push_back(FVector4(1.0f, 0.0f, 0.0f, 1.0f));  // 빨강
    axisIndices.push_back(0);
    axisIndices.push_back(1);

    // Y축 (초록)
    axisVertices.push_back(FVector(0.0f, 0.0f, 0.0f));       // 원점
    axisVertices.push_back(FVector(0.0f, Length, 0.0f));     // +Y
    axisColors.push_back(FVector4(0.0f, 1.0f, 0.0f, 1.0f));  // 초록
    axisColors.push_back(FVector4(0.0f, 1.0f, 0.0f, 1.0f));  // 초록
    axisIndices.push_back(2);
    axisIndices.push_back(3);

    // Z축 (파랑)
    axisVertices.push_back(FVector(0.0f, 0.0f, 0.0f));       // 원점
    axisVertices.push_back(FVector(0.0f, 0.0f, Length));     // +Z
    axisColors.push_back(FVector4(0.0f, 0.0f, 1.0f, 1.0f));  // 파랑
    axisColors.push_back(FVector4(0.0f, 0.0f, 1.0f, 1.0f));  // 파랑
    axisIndices.push_back(4);
    axisIndices.push_back(5);

    FMeshData* MeshData = new FMeshData();
    MeshData->Vertices = axisVertices;
    MeshData->Color = axisColors;
    MeshData->Indices = axisIndices;

    UStaticMesh* Mesh = NewObject<UStaticMesh>();
    Mesh->Load(MeshData, Device);
    Add<UStaticMesh>("Axis", Mesh);

    UMeshLoader::GetInstance().AddMeshData("Axis", MeshData);
}

void UResourceManager::CreateTextBillboardMesh()
{
    TArray<uint32> Indices;
    for (uint32 i = 0;i < 100;i++)
    {
        Indices.push_back(i * 4 + 0);
        Indices.push_back(i * 4 + 1);
        Indices.push_back(i * 4 + 2);

        Indices.push_back(i * 4 + 2);
        Indices.push_back(i * 4 + 1);
        Indices.push_back(i * 4 + 3);
    }


    //if(UResourceManager::GetInstance().Get<UMaterial>())
    const uint32 MaxQuads = 100; // capacity
    FMeshData* BillboardData = new FMeshData;
    BillboardData->Indices = Indices;
    // Reserve capacity for MaxQuads (4 vertices per quad)
    BillboardData->Vertices.resize(MaxQuads * 4);
    BillboardData->Color.resize(MaxQuads * 4);
    BillboardData->UV.resize(MaxQuads * 4);

    UTextQuad* Mesh = NewObject<UTextQuad>();
    Mesh->Load(BillboardData, Device);
    Add<UTextQuad>("TextBillboard", Mesh);
    UMeshLoader::GetInstance().AddMeshData("TextBillboard", BillboardData);
}

void UResourceManager::CreateBillboardMesh()
{
    TArray<uint32> Indices;
    for (uint32 i = 0; i < 100; i++)
    {
        Indices.push_back(i * 4 + 0);
        Indices.push_back(i * 4 + 1);
        Indices.push_back(i * 4 + 2);

        Indices.push_back(i * 4 + 2);
        Indices.push_back(i * 4 + 1);
        Indices.push_back(i * 4 + 3);
    }

    const uint32 MaxQuads = 100; // capacity
    FMeshData* BillboardData = new FMeshData;
    BillboardData->Indices = Indices;
    // Reserve capacity for MaxQuads (4 vertices per quad)
    BillboardData->Vertices.resize(MaxQuads * 4);
    BillboardData->Color.resize(MaxQuads * 4);
    BillboardData->UV.resize(MaxQuads * 4);

    UTextQuad* Mesh = NewObject<UTextQuad>();
    Mesh->Load(BillboardData, Device);
    Add<UTextQuad>("Billboard", Mesh);
    UMeshLoader::GetInstance().AddMeshData("Billboard", BillboardData);
}

void UResourceManager::CreateGridMesh(int N, const FString& FilePath)
{
    if (ResourceMap[FilePath])
    {
        return;
    }
    TArray<FVector> gridVertices;
    TArray<FVector4> gridColors;
    TArray<uint32> gridIndices;
    // Z축 방향 선
    for (int i = -N; i <= N; i++)
    {
        if (i == 0)
        {
            continue;
        }
        // 색 결정: 5의 배수면 흰색, 아니면 회색
        float r = 0.1f, g = 0.1f, b = 0.1f;
        if (i % 5 == 0) { r = g = b = 0.4f; }
        if (i % 10 == 0) { r = g = b = 1.0f; }

        // 정점 2개 추가 (Z축 방향 라인)
        gridVertices.push_back(FVector((float)i, 0.0f, (float)-N));
        gridVertices.push_back(FVector((float)i, 0.0f, (float)N));
        gridColors.push_back(FVector4(r, g, b, 1.0f));
        gridColors.push_back(FVector4(r, g, b, 1.0f));

        // 인덱스 추가
        uint32 base = static_cast<uint32>(gridVertices.size());
        gridIndices.push_back(base - 2);
        gridIndices.push_back(base - 1);
    }

    // X축 방향 선
    for (int j = -N; j <= N; j++)
    {
        if (j == 0)
        {
            continue;
        }
        // 색 결정: 5의 배수면 흰색, 아니면 회색
        float r = 0.1f, g = 0.1f, b = 0.1f;

        if (j % 5 == 0) { r = g = b = 0.4f; }
        if (j % 10 == 0) { r = g = b = 1.0f; }

        // 정점 2개 추가 (X축 방향 라인)
        gridVertices.push_back(FVector((float)-N, 0.0f, (float)j));
        gridVertices.push_back(FVector((float)N, 0.0f, (float)j));
        gridColors.push_back(FVector4(r, g, b, 1.0f));
        gridColors.push_back(FVector4(r, g, b, 1.0f));

        // 인덱스 추가
        uint32 base = static_cast<uint32>(gridVertices.size());
        gridIndices.push_back(base - 2);
        gridIndices.push_back(base - 1);
    }

    gridVertices.push_back(FVector(0.0f, 0.0f, (float)-N));
    gridVertices.push_back(FVector(0.0f, 0.0f, 0.0f));
    gridColors.push_back(FVector4(1.0f, 1.0f, 1.0f, 1.0f));
    gridColors.push_back(FVector4(1.0f, 1.0f, 1.0f, 1.0f));
    uint32 base = static_cast<uint32>(gridVertices.size());
    gridIndices.push_back(base - 2);
    gridIndices.push_back(base - 1);

    gridVertices.push_back(FVector((float)-N, 0.0f, 0.0f));
    gridVertices.push_back(FVector(0.0f, 0.0f, 0.0f));
    gridColors.push_back(FVector4(1.0f, 1.0f, 1.0f, 1.0f));
    gridColors.push_back(FVector4(1.0f, 1.0f, 1.0f, 1.0f));
    base = static_cast<uint32>(gridVertices.size());
    gridIndices.push_back(base - 2);
    gridIndices.push_back(base - 1);

    FMeshData* MeshData = new FMeshData();
    MeshData->Vertices = gridVertices;
    MeshData->Color = gridColors;
    MeshData->Indices = gridIndices;

    UStaticMesh* Mesh = NewObject<UStaticMesh>();
    Mesh->Load(MeshData, Device);
    Add<UStaticMesh>("Grid", Mesh);

    UMeshLoader::GetInstance().AddMeshData("Grid", MeshData);
}

void UResourceManager::CreateBoxWireframeMesh(const FVector& Min, const FVector& Max, const FString& FilePath)
{
    // 이미 있으면 패스
    if (ResourceMap[FilePath])
    {
        return;
    }

    TArray<FVector> vertices;
    TArray<FVector4> colors;
    TArray<uint32> indices;

    // ─────────────────────────────
    // 8개의 꼭짓점 (AABB)
    // ─────────────────────────────
    vertices.push_back(FVector(Min.X, Min.Y, Min.Z)); // 0
    vertices.push_back(FVector(Max.X, Min.Y, Min.Z)); // 1
    vertices.push_back(FVector(Max.X, Max.Y, Min.Z)); // 2
    vertices.push_back(FVector(Min.X, Max.Y, Min.Z)); // 3
    vertices.push_back(FVector(Min.X, Min.Y, Max.Z)); // 4
    vertices.push_back(FVector(Max.X, Min.Y, Max.Z)); // 5
    vertices.push_back(FVector(Max.X, Max.Y, Max.Z)); // 6
    vertices.push_back(FVector(Min.X, Max.Y, Max.Z)); // 7

    // 색상 (디버깅용 흰색)
    for (int i = 0; i < 8; i++)
    {
        colors.push_back(FVector4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // ─────────────────────────────
    // 12개의 선 (24 인덱스)
    // ─────────────────────────────
    uint32 boxIndices[] = {
        // 바닥
        0, 1, 1, 2, 2, 3, 3, 0,
        // 천장
        4, 5, 5, 6, 6, 7, 7, 4,
        // 기둥
        0, 4, 1, 5, 2, 6, 3, 7
    };

    indices.insert(indices.end(), std::begin(boxIndices), std::end(boxIndices));

    // ─────────────────────────────
    // MeshData 생성 및 등록
    // ─────────────────────────────
    FMeshData* MeshData = new FMeshData();
    MeshData->Vertices = vertices;
    MeshData->Color = colors;
    MeshData->Indices = indices;

    UStaticMesh* Mesh = NewObject<UStaticMesh>();
    Mesh->Load(MeshData, Device);
    //Mesh->SetTopology(EPrimitiveTopology::LineList); // ✅ 꼭 LineList로 설정

    Add<UStaticMesh>(FilePath, Mesh);

    UMeshLoader::GetInstance().AddMeshData(FilePath, MeshData);
}


//해당 과정과 동일하게 생성된 메쉬는 문제가 있음
// FStaticMesh가 아닌 FMeshData로 생성되고 UStaticMesh의 FStaticMesh가 비어있게됨
void UResourceManager::CreateScreenQuatMesh()
{
    TArray<FVector> Vertices = { FVector(-1.0f,-1.0f,0.0f),FVector(-1.0f,1.0f,0.0f) ,FVector(1.0f,1.0f,0.0f) ,FVector(1.0f,-1.0f,0.0f) };
    TArray<FVector2D> UVs = { FVector2D(0.0f,1.0f),FVector2D(0.0f,0.0f) ,FVector2D(1.0f,0.0f) ,FVector2D(1.0f,1.0f) };
    TArray<uint32> Indices = { 0,1,2,0,2,3 };

    UStaticMesh* Mesh = NewObject<UStaticMesh>();
    FMeshData* MeshData = new FMeshData();
    MeshData->Vertices = Vertices;
    MeshData->UV = UVs;
    MeshData->Indices = Indices;
    Mesh->Load(MeshData, Device);
    //Mesh->SetTopology(EPrimitiveTopology::LineList); // :흰색_확인_표시: 꼭 LineList로 설정
    Add<UStaticMesh>("ScreenQuad", Mesh);
    UMeshLoader::GetInstance().AddMeshData("ScreenQuad", MeshData);
}


void UResourceManager::InitShaderILMap()
{
    TArray<D3D11_INPUT_ELEMENT_DESC> layout;
    layout.Add({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    ShaderToInputLayoutMap["FXAA.hlsl"] = layout;
    ShaderToInputLayoutMap["Copy.hlsl"] = layout;
    layout.clear();

    layout.Add({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    ShaderToInputLayoutMap["ShaderLine.hlsl"] = layout;
    ShaderToInputLayoutMap["Primitive.hlsl"] = layout;
    layout.clear();

    layout.Add({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    ShaderToInputLayoutMap["StaticMeshShader.hlsl"] = layout;
    ShaderToInputLayoutMap["WorldNormalShader.hlsl"] = layout;
    ShaderToInputLayoutMap["UberLit.hlsl"] = layout;
    ShaderToInputLayoutMap["DecalShader.hlsl"] = layout;
    ShaderToInputLayoutMap["DecalSpotLightShader.hlsl"] = layout;
    layout.clear();

    layout.Add({ "WORLDPOSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "UVRECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    ShaderToInputLayoutMap["TextBillboard.hlsl"] = layout;
    ShaderToInputLayoutMap["Billboard.hlsl"] = layout;
    layout.clear();

    layout.Add({ "WORLDPOSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "UVRECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    ShaderToInputLayoutMap["TextShader.hlsl"] = layout;
    layout.clear();

    // DepthPrepassShader uses same layout as UberLit.hlsl (POSITION, NORMAL, COLOR, TEXCOORD, TANGENT)
    layout.Add({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.Add({ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    ShaderToInputLayoutMap["DepthPrepassShader.hlsl"] = layout;
    layout.clear();
    ShaderToInputLayoutMap["DepthVisualizeShader.hlsl"] = layout;
}

/**
 * @brief Data/ 폴더 전체에서 노멀맵 파일(_n, _normal 등)을 스캔합니다.
 *              BuildFilteredResourceLists() 내부에서만 사용됩니다.
 */
static TArray<FString> ScanForNormalMapFiles()
{
    TArray<FString> NormalMapFiles;
    UE_LOG("Scanning for normal map files...");

    try
    {
        std::filesystem::path DataPath = FObjManager::GetFullDataPath("");
        if (std::filesystem::exists(DataPath) && std::filesystem::is_directory(DataPath))
        {
            for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataPath))
            {
                // 현재 진입점이 디렉토리나 특수 파일과 같은 곳이라면 건너뜁니다.
                if (!Entry.is_regular_file()) { continue; }

                // 현재 파일의 문자열을 소문자로 변환하고 비교할 준비를 합니다.
                const std::filesystem::path& Path = Entry.path();
                FString Extension = Path.extension().string();
                std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::tolower);

                if (Extension == ".dds" || Extension == ".png" || Extension == ".jpg" || Extension == ".jpeg")
                {
                    // 확장자를 제외한 파일 이름만 추출을 하고, 문자열을 소문자로 변환합니다.
                    FString CurrentFileName = Path.stem().string();
                    std::transform(CurrentFileName.begin(), CurrentFileName.end(), CurrentFileName.begin(), ::tolower);

                    // 파일이 노말맵 텍스쳐 유형에 해당된다면 아래 분기문을 실행합니다.
                    if (bool isNormalMap = (CurrentFileName.length() >= 4 && CurrentFileName.substr(CurrentFileName.length() - 4) == "_nrm"))
                    {
                        // 현재 파일 경로를 Data 폴더 기준 상대 경로로 변환을 시도합니다.
                        std::error_code Error;
                        std::filesystem::path RelativePath = std::filesystem::relative(Path, DataPath, Error);

                        // 실패한다면 원본 경로 (절대 경로)를 그대로 사용합니다.
                        if (Error) { RelativePath = Path; }

                        FString RelativePathStr = RelativePath.string();
                        std::replace(RelativePathStr.begin(), RelativePathStr.end(), '\\', '/');
                        NormalMapFiles.push_back("Data/" + RelativePathStr);
                    }
                }
            }
        }
    }
    catch (const std::exception& E)
    {
        UE_LOG("Failed to scan Data directory: %s", E.what());
    }

    return NormalMapFiles;
}

static TArray<FString> ScanForMaterialBinFiles()
{
    TArray<FString> MaterialBinFiles;
    UE_LOG("Scanning for material files...");

    try
    {
        std::filesystem::path DataPath = FObjManager::GetFullDataPath("");
        if (std::filesystem::exists(DataPath) && std::filesystem::is_directory(DataPath))
        {
            for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataPath))
            {
                if (!Entry.is_regular_file()) { continue; }

                const std::filesystem::path& Path = Entry.path();
                FString FileName = Path.filename().string();

                if (FileName.length() >= 7 && FileName.substr(FileName.length() - 7) == "Mat.bin")
                {
                    std::error_code Error;
                    std::filesystem::path RelativePath = std::filesystem::relative(Path, DataPath, Error);

                    if (Error) { RelativePath = Path; }

                    FString RelativePathStr = RelativePath.string();
                    std::replace(RelativePathStr.begin(), RelativePathStr.end(), '\\', '/');
                    MaterialBinFiles.push_back("Data/" + RelativePathStr);
                }
            }
        }
    }
    catch (const std::exception& E)
    {
        UE_LOG("Failed to scan Data directory for materials: %s", E.what());
    }

    return MaterialBinFiles;
}

void UResourceManager::BuildFilteredResourceLists()
{
    UE_LOG("Building filtered resource lists for UI...");

    auto IsGizmoPath = [](const FString& Path) -> bool
        {
            FString Lower = Path;
            std::transform(Lower.begin(), Lower.end(), Lower.begin(), ::tolower);
            return (Lower.find("data/gizmo/") != std::string::npos);
        };

    // 1. Static Mesh (.obj) 필터링
    FilteredObjPaths.clear();
    const TArray<FString> AllMeshPaths = GetAllStaticMeshFilePaths(); 
    for (const FString& Path : AllMeshPaths)
    {
        if (Path == "None" || IsGizmoPath(Path))
            continue;

        std::filesystem::path FsPath(Path);
        FString Extension = FsPath.extension().string();
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::tolower);
        if (Extension == ".obj")
        {
            FilteredObjPaths.push_back(Path);
        }
    }

    // 2. Shader (.hlsl) 필터링
    FilteredHlslPaths.clear();
    const TArray<FString> AllShaderPaths = GetAllFilePaths<UShader>();
    for (const FString& Path : AllShaderPaths)
    {
        if (Path == "None" || IsGizmoPath(Path))
            continue;

        std::filesystem::path fsPath(Path);
        FString Extension = fsPath.extension().string();
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::tolower);
        if (Extension == ".hlsl")
        {
            FilteredHlslPaths.push_back(Path);
        }
    }

    // 3. Material 필터링 (*Mat.bin 파일 스캔)
    FilteredMaterialPaths.clear();
    FilteredMaterialPaths.push_back("None"); // "None" 옵션 추가
    const TArray<FString> AllMaterialPaths = ScanForMaterialBinFiles();
    for (const FString& Path : AllMaterialPaths)
    {
        if (IsGizmoPath(Path)) // Gizmo 관련 경로 제외
            continue;

        FilteredMaterialPaths.push_back(Path);
    }

    // 4. Normal Map (파일 시스템 스캔)
    FilteredNormalMapPaths.clear();
    FilteredNormalMapPaths.push_back("None"); // "None" 옵션 추가
    const TArray<FString> AllNormalMaps = ScanForNormalMapFiles();
    for (const FString& Path : AllNormalMaps)
    {
        if (Path == "None" || IsGizmoPath(Path)) // "None"은 이미 추가했으므로, 여기서는 스킵
            continue;

        FilteredNormalMapPaths.push_back(Path);
    }

    UE_LOG("Filtered UI lists built. Mesh: %u, Shader: %u, Material: %u, Normal: %u",
           static_cast<uint32>(FilteredObjPaths.size()),
           static_cast<uint32>(FilteredHlslPaths.size()),
           static_cast<uint32>(FilteredMaterialPaths.size()),
           static_cast<uint32>(FilteredNormalMapPaths.size()));
}

TArray<D3D11_INPUT_ELEMENT_DESC>& UResourceManager::GetProperInputLayout(const FString& InShaderName)
{
    auto it = ShaderToInputLayoutMap.find(InShaderName);

    if (it == ShaderToInputLayoutMap.end())
    {
        TArray<D3D11_INPUT_ELEMENT_DESC> Empty;
        return Empty;
        //throw std::runtime_error("Proper input layout not found for " + InShaderName);
    }
    
    return ShaderToInputLayoutMap[InShaderName];
}

FString& UResourceManager::GetProperShader(const FString& InTextureName)
{
    auto it = TextureToShaderMap.find(InTextureName);

    if (it == TextureToShaderMap.end())
    {
        throw std::runtime_error("Proper shader not found for " + InTextureName);
    }

    return TextureToShaderMap[InTextureName];
}


void UResourceManager::CreateTextBillboardTexture()
{
    UTexture* TextBillboardTexture = NewObject<UTexture>();
    TextBillboardTexture->Load("TextBillboard.dds",Device);
    Add<UTexture>("TextBillboard.dds", TextBillboardTexture);
}


void UResourceManager::UpdateDynamicVertexBuffer(const FString& Name, TArray<FBillboardVertexInfo_GPU>& vertices)
{
    UTextQuad* Mesh = Get<UTextQuad>(Name);

    UpdateDynamicVertexBuffer(Mesh, vertices);
}

void UResourceManager::UpdateDynamicVertexBuffer(UTextQuad* InMesh, TArray<FBillboardVertexInfo_GPU>& InVertices)
{
    const uint32_t quadCount = static_cast<uint32_t>(InVertices.size() / 4);
    InMesh->SetIndexCount(quadCount * 6);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Context->Map(InMesh->GetVertexBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, InVertices.data(), sizeof(FBillboardVertexInfo_GPU) * InVertices.size()); //vertices.size()만큼의 Character info를 vertices에서 pData로 복사해가라
    Context->Unmap(InMesh->GetVertexBuffer(), 0);
}

FTextureData* UResourceManager::CreateOrGetTextureData(const FName& FileName)
{
    if (TextureMap.count(FileName))
    {
        return TextureMap[FileName];
    }
    
    const int RequiredSize = MultiByteToWideChar(CP_UTF8, 0, FileName.ToString().c_str(), -1, NULL, 0);

    FWideString OutWstring;
    OutWstring.resize(RequiredSize);
    MultiByteToWideChar(CP_UTF8, 0, FileName.ToString().c_str(), -1, &OutWstring[0], RequiredSize);

    OutWstring.resize(RequiredSize - 1);
    FWideString FilePath = OutWstring;
    if (FilePath.empty() ) {
		return nullptr;
    }
    FTextureData* Data = new FTextureData();

    const FWideString& FileExtension = FilePath.substr(FilePath.size() - 3);
    HRESULT hr;
    if (FileExtension == L"dds")
    {
        hr = DirectX::CreateDDSTextureFromFile(Device, FilePath.c_str(), &Data->Texture, &Data->TextureSRV, 0, nullptr);
    }
    else
    {
        hr = DirectX::CreateWICTextureFromFile(Device, Context, FilePath.c_str(), &Data->Texture, &Data->TextureSRV);
    }

    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    // 멤버 변수 m_pAlphaBlendState에 저장
    hr = Device->CreateBlendState(&blendDesc, &Data->BlendState);
    if (FAILED(hr))
    {

    }
    TextureMap[FileName] = Data;
    return TextureMap[FileName];
}
