#include "pch.h"
#include "MeshLoader.h"

UMeshLoader& UMeshLoader::GetInstance()
{
    static UMeshLoader* Instance = nullptr;
    if (Instance == nullptr)
    {
        Instance = NewObject<UMeshLoader>();
    }
    return *Instance;
}

UMeshLoader::FFace UMeshLoader::ParseFaceBuffer(const FString& FaceBuffer)
{
    FFace Face = {};
    std::istringstream Tokenizer(FaceBuffer);
    FString IndexBuffer;

    // v (항상 존재)
    if (std::getline(Tokenizer, IndexBuffer, '/'))
        Face.IndexPosition = std::stoi(IndexBuffer);

    // vt (선택적)
    if (std::getline(Tokenizer, IndexBuffer, '/'))
    {
        if (!IndexBuffer.empty())
            Face.IndexTexCoord = std::stoi(IndexBuffer);
    }

    // vn (선택적)
    if (std::getline(Tokenizer, IndexBuffer, '/'))
    {
        if (!IndexBuffer.empty())
            Face.IndexNormal = std::stoi(IndexBuffer);
    }

    return Face;
}

UMeshLoader::~UMeshLoader()
{
    for (auto& it : MeshCache)
    {
        if (it.second)
        {
            delete it.second;
        }
        it.second = nullptr;
    }
    MeshCache.Empty();
}
FMeshData* UMeshLoader::LoadMesh(const std::filesystem::path& FilePath)
{
    auto it = MeshCache.find(FilePath.string());
    if (it != MeshCache.end())
    {
        return it->second;
    }

    if (!std::filesystem::exists(FilePath))
        return nullptr;

    std::ifstream File(FilePath);
    if (!File)
        return nullptr;

    TArray<FPosition> Positions;
    TArray<FNormal> Normals;
    TArray<FTexCoord> TexCoords;
    TArray<FFace> Faces;

    FString Line;
    while (std::getline(File, Line))
    {
        std::istringstream Tokenizer(Line);

        FString Prefix;
        Tokenizer >> Prefix;

        if (Prefix == "v") // position
        {
            FPosition Position;
            Tokenizer >> Position.x >> Position.y >> Position.z;
            Positions.push_back(Position);
        }
        else if (Prefix == "vn") // normal
        {
            FNormal Normal;
            Tokenizer >> Normal.x >> Normal.y >> Normal.z;
            Normals.push_back(Normal);
        }
        else if (Prefix == "vt") // uv
        {
            FTexCoord TexCoord;
            Tokenizer >> TexCoord.u >> TexCoord.v;
            TexCoords.push_back(TexCoord);
        }
        else if (Prefix == "f") // face
        {
            FString FaceBuffer;
            while (Tokenizer >> FaceBuffer)
            {
                Faces.push_back(ParseFaceBuffer(FaceBuffer));
            }
        }
        else if (Prefix == "l") // line (바운딩박스 OBJ용)
        {
            FString LineBuffer;
            while (Tokenizer >> LineBuffer)
            {
                Faces.push_back(ParseFaceBuffer(LineBuffer));
            }
        }
    }
    FMeshData* MeshData = new FMeshData();
    TMap<FVertexKey, uint32> UniqueVertexMap;

    for (const auto& Face : Faces)
    {
        FVertexKey key{ Face.IndexPosition, Face.IndexNormal, Face.IndexTexCoord };

        auto it = UniqueVertexMap.find(key);
        if (it != UniqueVertexMap.end())
        {
            MeshData->Indices.push_back(it->second);
        }
        else
        {
            uint32 newIndex = static_cast<uint32>(MeshData->Vertices.size());

            // 위치 데이터 추가
            const FPosition& Pos = Positions[Face.IndexPosition - 1];
            MeshData->Vertices.push_back(FVector(Pos.x, Pos.y, Pos.z));

            // UV 데이터 추가
            if (Face.IndexTexCoord > 0 && Face.IndexTexCoord <= TexCoords.size())
            {
                const FTexCoord& UV = TexCoords[Face.IndexTexCoord - 1];
                MeshData->UV.push_back(FVector2D(UV.u, UV.v));
            }
            else
            {
                MeshData->UV.push_back(FVector2D(0.0f, 0.0f));
            }

            // Normal 데이터 추가
            if (Face.IndexNormal > 0 && Face.IndexNormal <= Normals.size())
            {
                const FNormal& Norm = Normals[Face.IndexNormal - 1];
                MeshData->Normal.push_back(FVector(Norm.x, Norm.y, Norm.z));
            }
            else
            {
                // Normal이 없는 경우 기본값 (0, 1, 0)
                MeshData->Normal.push_back(FVector(0.0f, 1.0f, 0.0f));
            }

            // 랜덤 색상 추가
            MeshData->Color.push_back(FVector4(
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                1.0f
            ));

            MeshData->Indices.push_back(newIndex);
            UniqueVertexMap[key] = newIndex;
        }
    }

    // Tangent 계산
    MeshData->Tangent.resize(MeshData->Vertices.size(), FVector(0.f, 0.f, 0.f));

    for (size_t i = 0; i < MeshData->Indices.size(); i += 3)
    {
        uint32 i0 = MeshData->Indices[i];
        uint32 i1 = MeshData->Indices[i + 1];
        uint32 i2 = MeshData->Indices[i + 2];

        const FVector& v0 = MeshData->Vertices[i0];
        const FVector& v1 = MeshData->Vertices[i1];
        const FVector& v2 = MeshData->Vertices[i2];

        const FVector2D& uv0 = MeshData->UV[i0];
        const FVector2D& uv1 = MeshData->UV[i1];
        const FVector2D& uv2 = MeshData->UV[i2];

        FVector deltaPos1 = v1 - v0;
        FVector deltaPos2 = v2 - v0;

        FVector2D deltaUV1 = uv1 - uv0;
        FVector2D deltaUV2 = uv2 - uv0;

        float r = 1.0f / (deltaUV1.X * deltaUV2.Y - deltaUV1.Y * deltaUV2.X);

        if (isinf(r) || isnan(r))
        {
            continue;
        }

        FVector tangent = (deltaPos1) * r;

        MeshData->Tangent[i0] += tangent;
        MeshData->Tangent[i1] += tangent;
        MeshData->Tangent[i2] += tangent;
    }

    for (size_t i = 0; i < MeshData->Vertices.size(); ++i)
    {
        const FVector& n = MeshData->Normal[i];
        const FVector& t = MeshData->Tangent[i];

        // Gram-Schmidt 직교화
        FVector tangent = (t - n * n.Dot(t)).GetSafeNormal();

        MeshData->Tangent[i] = tangent;
    }

    MeshCache[FilePath.string()] = MeshData;

    return MeshData;
}

void UMeshLoader::AddMeshData(const FString& InName, FMeshData* InMeshData)
{
    if (MeshCache.Contains(InName))
    {
        delete MeshCache[InName];
    }
    MeshCache[InName] = InMeshData;
}

const TMap<FString, FMeshData*>* UMeshLoader::GetMeshCache()
{
    return &MeshCache;
}

UMeshLoader::UMeshLoader()
{
}
