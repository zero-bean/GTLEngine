#pragma once
#include "Archive.h"
#include "Vector.h"
#include "Name.h"

// 직렬화 포맷 (FVertexDynamic와 역할이 달라서 분리됨)
struct FNormalVertex
{
    FVector pos;
    FVector normal;
    FVector2D tex;
    FVector4 Tangent;
    FVector4 color;

    friend FArchive& operator<<(FArchive& Ar, FNormalVertex& Vtx)
    {
        Ar << Vtx.pos;
        Ar << Vtx.normal;
        Ar << Vtx.Tangent;
        Ar << Vtx.color;
        Ar << Vtx.tex;
        return Ar;
    }
};

struct FMeshData
{
    TArray<FVector> Vertices;
    TArray<uint32> Indices;
    TArray<FVector4> Color;
    TArray<FVector2D> UV;
    TArray<FVector> Normal;
};

struct FVertexSimple
{
    FVector Position;
    FVector4 Color;

    void FillFrom(const FMeshData& mesh, size_t i);
    void FillFrom(const FNormalVertex& src);
};

// 런타임 포맷 (FNormalVertex와 역할이 달라서 분리됨)
struct FVertexDynamic
{
    FVector Position;
    FVector Normal;
    FVector2D UV;
    FVector4 Tangent;
    FVector4 Color;

    void FillFrom(const FMeshData& mesh, size_t i);
    void FillFrom(const FNormalVertex& src);
};

/**
* 스키닝용 정점 구조체
*/
struct FSkinnedVertex
{
    FVector Position{}; // 정점 위치
    FVector Normal{}; // 법선 벡터
    FVector2D UV{}; // 텍스처 좌표
    FVector4 Tangent{}; // 탄젠트 (w는 binormal 방향)
    FVector4 Color{}; // 정점 컬러
    uint32 BoneIndices[4]{}; // 영향을 주는 본 인덱스 (최대 4개)
    float BoneWeights[4]{}; // 각 본의 가중치 (합이 1.0)

    friend FArchive& operator<<(FArchive& Ar, FSkinnedVertex& Vertex)
    {
        Ar << Vertex.Position;
        Ar << Vertex.Normal;
        Ar << Vertex.UV;
        Ar << Vertex.Tangent;
        Ar << Vertex.Color;

        for (int i = 0; i < 4; ++i)
        {
            Ar << Vertex.BoneIndices[i];
        }

        for (int i = 0; i < 4; ++i)
        {
            Ar << Vertex.BoneWeights[i];
        }

        return Ar;
    }
};

// 같은 Position인데 Normal이나 UV가 다른 vertex가 존재할 수 있음, 그래서 SkinnedVertex를 키로 구별해야해서 hash함수 정의함
template <class T>
inline void CombineHash(size_t& InSeed, const T& Vertex)
{
    std::hash<T> Hasher;

    InSeed ^= Hasher(Vertex) + 0x9e3779b9 + (InSeed << 6) + (InSeed >> 2);
}
inline bool operator==(const FSkinnedVertex& Vertex1, const FSkinnedVertex& Vertex2)
{
    if (Vertex1.Position != Vertex2.Position ||
        Vertex1.Normal != Vertex2.Normal ||
        Vertex1.UV != Vertex2.UV ||
        Vertex1.Tangent != Vertex2.Tangent ||
        Vertex1.Color != Vertex2.Color)
    {
        return false;
    }

    if (std::memcmp(Vertex1.BoneIndices, Vertex2.BoneIndices, sizeof(Vertex1.BoneIndices)) != 0)
    {
        return false;
    }

    if (std::memcmp(Vertex1.BoneWeights, Vertex2.BoneWeights, sizeof(Vertex1.BoneWeights)) != 0)
    {
        return false;
    }

    // 모든 게 같음
    return true;
}
namespace std
{
    template<>
    struct hash<FSkinnedVertex>
    {

        size_t operator()(const FSkinnedVertex& Vertex) const
        {
            size_t Seed = 0;

            CombineHash(Seed, Vertex.Position.X); CombineHash(Seed, Vertex.Position.Y); CombineHash(Seed, Vertex.Position.Z);
            CombineHash(Seed, Vertex.Normal.X); CombineHash(Seed, Vertex.Normal.Y); CombineHash(Seed, Vertex.Normal.Z);
            CombineHash(Seed, Vertex.Tangent.X); CombineHash(Seed, Vertex.Tangent.Y); CombineHash(Seed, Vertex.Tangent.Z); CombineHash(Seed, Vertex.Tangent.W);
            CombineHash(Seed, Vertex.Color.X); CombineHash(Seed, Vertex.Color.Y); CombineHash(Seed, Vertex.Color.Z); CombineHash(Seed, Vertex.Color.W);
            CombineHash(Seed, Vertex.UV.X); CombineHash(Seed, Vertex.UV.Y);

            for (int Index = 0; Index < 4; Index++)
            {
                CombineHash(Seed, Vertex.BoneIndices[Index]);
                CombineHash(Seed, Vertex.BoneWeights[Index]);
            }
            return Seed;
        }
    };
}

struct FBillboardVertexInfo {
    FVector WorldPosition;
    FVector2D CharSize;//char scale
    FVector4 UVRect;//uv start && uv size
};

struct FBillboardVertexInfo_GPU
{
    float Position[3];
    float CharSize[2];
    float UVRect[4];

    void FillFrom(const FMeshData& mesh, size_t i); 
    void FillFrom(const FNormalVertex& src);
};

// 빌보드 전용: 위치 + UV만 있으면 충분
struct FBillboardVertex
{
    FVector WorldPosition;  // 정점 위치 (로컬 좌표, -0.5~0.5 기준 쿼드)
    FVector2D UV;        // 텍스처 좌표 (0~1)

    void FillFrom(const FMeshData& mesh, size_t i);
    void FillFrom(const FNormalVertex& src);
};

struct FGroupInfo
{
    uint32 StartIndex = 0;
    uint32 IndexCount = 0;
    FString InitialMaterialName; // obj 파일 자체에 맵핑된 material 이름

    friend FArchive& operator<<(FArchive& Ar, FGroupInfo& Info)
    {
        Ar << Info.StartIndex;
        Ar << Info.IndexCount;

        if (Ar.IsSaving())
            Serialization::WriteString(Ar, Info.InitialMaterialName);
        else if (Ar.IsLoading())
            Serialization::ReadString(Ar, Info.InitialMaterialName);

        return Ar;
    }
};

struct FStaticMesh
{
    FString PathFileName;
    FString CacheFilePath;  // 캐시된 소스 경로 (예: DerivedDataCache/cube.obj.bin)

    TArray<uint32> Indices;
    TArray<FNormalVertex> Vertices;
    TArray<FGroupInfo> GroupInfos; // 각 group을 render 하기 위한 정보

    bool bHasMaterial;

    friend FArchive& operator<<(FArchive& Ar, FStaticMesh& Mesh)
    {
        if (Ar.IsSaving())
        {
            Serialization::WriteString(Ar, Mesh.PathFileName);
            Serialization::WriteArray(Ar, Mesh.Vertices);
            Serialization::WriteArray(Ar, Mesh.Indices);

            uint32_t gCount = (uint32_t)Mesh.GroupInfos.size();
            Ar << gCount;
            for (auto& g : Mesh.GroupInfos) Ar << g;

            Ar << Mesh.bHasMaterial;
        }
        else if (Ar.IsLoading())
        {
            Serialization::ReadString(Ar, Mesh.PathFileName);
            Serialization::ReadArray(Ar, Mesh.Vertices);
            Serialization::ReadArray(Ar, Mesh.Indices);

            uint32_t gCount;
            Ar << gCount;
            Mesh.GroupInfos.resize(gCount);
            for (auto& g : Mesh.GroupInfos) Ar << g;

            Ar << Mesh.bHasMaterial;
        }
        return Ar;
    }
};

struct FBone
{
    FString Name; // 본 이름
    int32 ParentIndex; // 부모 본 인덱스 (-1이면 루트)
    FMatrix BindPose; // Bind Pose 변환 행렬
    FMatrix InverseBindPose; // Inverse Bind Pose (스키닝용)

    friend FArchive& operator<<(FArchive& Ar, FBone& Bone)
    {
        if (Ar.IsSaving())
        {
            Serialization::WriteString(Ar, Bone.Name);
            Ar << Bone.ParentIndex;
            Ar << Bone.BindPose;
            Ar << Bone.InverseBindPose;
        }
        else if (Ar.IsLoading())
        {
            Serialization::ReadString(Ar, Bone.Name);
            Ar << Bone.ParentIndex;
            Ar << Bone.BindPose;
            Ar << Bone.InverseBindPose;
        }
        return Ar;
    }
};

struct FSkeleton
{
    FString Name; // 스켈레톤 이름
    TArray<FBone> Bones; // 본 배열
    TMap <FString, int32> BoneNameToIndex; // 이름으로 본 검색

    friend FArchive& operator<<(FArchive& Ar, FSkeleton& Skeleton)
    {
        if (Ar.IsSaving())
        {
            Serialization::WriteString(Ar, Skeleton.Name);

            uint32 boneCount = static_cast<uint32>(Skeleton.Bones.size());
            Ar << boneCount;
            for (auto& bone : Skeleton.Bones)
            {
                Ar << bone;
            }
            // BoneNameToIndex는 로드 시 재구축 가능하므로 저장 안 함
        }
        else if (Ar.IsLoading())
        {
            Serialization::ReadString(Ar, Skeleton.Name);

            uint32 boneCount;
            Ar << boneCount;
            Skeleton.Bones.resize(boneCount);
            for (auto& bone : Skeleton.Bones)
            {
                Ar << bone;
            }

            // BoneNameToIndex 재구축
            Skeleton.BoneNameToIndex.clear();
            for (int32 i = 0; i < static_cast<int32>(Skeleton.Bones.size()); ++i)
            {
                Skeleton.BoneNameToIndex[Skeleton.Bones[i].Name] = i;
            }
        }
        return Ar;
    }
};

struct FVertexWeight
{
    uint32 VertexIndex; // 정점 인덱스
    float Weight; // 가중치
};

struct FSkeletalMeshData
{
    FString PathFileName;
    FString CacheFilePath;
    
    TArray<FSkinnedVertex> Vertices; // 정점 배열
    TArray<uint32> Indices; // 인덱스 배열
    FSkeleton Skeleton; // 스켈레톤 정보
    TArray<FGroupInfo> GroupInfos; // 머티리얼 그룹 (기존 시스템 재사용)
    bool bHasMaterial = false;

    friend FArchive& operator<<(FArchive& Ar, FSkeletalMeshData& Data)
    {
        if (Ar.IsSaving())
        {
            // 1. Vertices 저장
            Serialization::WriteArray(Ar, Data.Vertices);

            // 2. Indices 저장
            Serialization::WriteArray(Ar, Data.Indices);

            // 3. Skeleton 저장
            Ar << Data.Skeleton;

            // 4. GroupInfos 저장
            uint32 gCount = static_cast<uint32>(Data.GroupInfos.size());
            Ar << gCount;
            for (auto& g : Data.GroupInfos)
            {
                Ar << g;
            }

            // 5. Material 플래그 저장
            Ar << Data.bHasMaterial;

            // 6. CacheFilePath 저장
            Serialization::WriteString(Ar, Data.CacheFilePath);
        }
        else if (Ar.IsLoading())
        {
            // 1. Vertices 로드
            Serialization::ReadArray(Ar, Data.Vertices);

            // 2. Indices 로드
            Serialization::ReadArray(Ar, Data.Indices);

            // 3. Skeleton 로드
            Ar << Data.Skeleton;

            // 4. GroupInfos 로드
            uint32 gCount;
            Ar << gCount;
            Data.GroupInfos.resize(gCount);
            for (auto& g : Data.GroupInfos)
            {
                Ar << g;
            }

            // 5. Material 플래그 로드
            Ar << Data.bHasMaterial;

            // 6. CacheFilePath 로드
            Serialization::ReadString(Ar, Data.CacheFilePath);
        }
        return Ar;
    }
};

struct FFrameRate
{
    int32 Numerator = 30;   // 분자
    int32 Denominator = 1;  // 분모

    float AsDecimal() const
    {
        if (Denominator == 0) { return 0.f; }
        return static_cast<float>(Numerator) / Denominator;
    }

    int32 AsFrameNumber(float TimeInSeconds) const
    {
        const float Decimal = AsDecimal();
        return Decimal > KINDA_SMALL_NUMBER ? static_cast<int32>(std::round(TimeInSeconds * Decimal)) : 0;
    }

    float AsSeconds(int32 FrameNumber) const
    {
        const float Decimal = AsDecimal();
        return Decimal > KINDA_SMALL_NUMBER ? static_cast<float>(FrameNumber) / Decimal : 0.f;
    }

    bool IsValid() const { return Numerator > 0 && Denominator > 0; }

    friend FArchive& operator<<(FArchive& Ar, FFrameRate& Rate)
    {
        Ar << Rate.Numerator;
        Ar << Rate.Denominator;
        return Ar;
    }
};

// FQuat 직렬화 (Vector.h에 정의되어 있지만 operator<<는 여기 추가)
inline FArchive& operator<<(FArchive& Ar, FQuat& Q)
{
    Ar.Serialize(&Q.X, sizeof(float));
    Ar.Serialize(&Q.Y, sizeof(float));
    Ar.Serialize(&Q.Z, sizeof(float));
    Ar.Serialize(&Q.W, sizeof(float));
    return Ar;
}

// FName 직렬화
inline FArchive& operator<<(FArchive& Ar, FName& Name)
{
    if (Ar.IsSaving())
    {
        // FName을 문자열로 변환해서 저장
        FString Str = Name.ToString();
        Serialization::WriteString(Ar, Str);
    }
    else if (Ar.IsLoading())
    {
        // 문자열을 읽어서 FName으로 변환
        FString Str;
        Serialization::ReadString(Ar, Str);
        Name = FName(Str);
    }
    return Ar;
}

struct FRawAnimSequenceTrack
{
    TArray<FVector> PosKeys;    // 위치 키프레임
    TArray<FQuat>   RotKeys;    // 회전 키프레임
    TArray<FVector> ScaleKeys;  // 스케일 키프레임
    TArray<float>   KeyTimes;   // 키 타임(초). 불규칙한 타이밍을 그대로 보관.

    int32 GetNumKeys() const
    {
        if (KeyTimes.empty() == false)
        {
            return static_cast<int32>(KeyTimes.size());
        }

        const int32 NumPosKeys = static_cast<int32>(PosKeys.size());
        const int32 NumRotKeys = static_cast<int32>(RotKeys.size());
        const int32 NumScaleKeys = static_cast<int32>(ScaleKeys.size());
        return std::max({ NumPosKeys, NumRotKeys, NumScaleKeys });
    }

    bool HasAnyKeys() const
    {
        return !PosKeys.empty() || !RotKeys.empty() || !ScaleKeys.empty() || !KeyTimes.empty();
    }

    FTransform GetTransform(float FrameRate, float Time) const
    {
        uint32 KeyCount = PosKeys.Num();
        float MaxTime = KeyCount / FrameRate;
        Time = Time < 0 ? 0 : (Time > MaxTime ? MaxTime : Time); //clamp(Time, 0, MaxTime);
        float TimeValue = Time * FrameRate;

        int PrevIdx = floor(TimeValue);
        int NextIdx = PrevIdx + 1;
        PrevIdx = Clamp(PrevIdx, 0, KeyCount - 1);
        NextIdx = Clamp(NextIdx, 0, KeyCount - 1);
        float T = TimeValue - PrevIdx;

        FTransform PrevTransform = FTransform(PosKeys[PrevIdx], RotKeys[PrevIdx], ScaleKeys[PrevIdx]);
        FTransform NextTransform = FTransform(PosKeys[NextIdx], RotKeys[NextIdx], ScaleKeys[NextIdx]);

        return FTransform::Lerp(PrevTransform, NextTransform, T);
    }

    friend FArchive& operator<<(FArchive& Ar, FRawAnimSequenceTrack& Track)
    {
        if (Ar.IsSaving())
        {
            // PosKeys
            uint32 PosCount = static_cast<uint32>(Track.PosKeys.size());
            Ar << PosCount;
            for (auto& Pos : Track.PosKeys) { Ar << Pos; }

            // RotKeys
            uint32 RotCount = static_cast<uint32>(Track.RotKeys.size());
            Ar << RotCount;
            for (auto& Rot : Track.RotKeys) { Ar << Rot; }

            // ScaleKeys
            uint32 ScaleCount = static_cast<uint32>(Track.ScaleKeys.size());
            Ar << ScaleCount;
            for (auto& Scale : Track.ScaleKeys) { Ar << Scale; }

            // KeyTimes
            uint32 TimeCount = static_cast<uint32>(Track.KeyTimes.size());
            Ar << TimeCount;
            for (auto& Time : Track.KeyTimes) { Ar.Serialize(&Time, sizeof(float)); }
        }
        else if (Ar.IsLoading())
        {
            // PosKeys
            uint32 PosCount = 0;
            Ar << PosCount;
            Track.PosKeys.clear();
            Track.PosKeys.reserve(PosCount);
            for (uint32 i = 0; i < PosCount; ++i)
            {
                FVector Pos;
                Ar << Pos;
                Track.PosKeys.push_back(Pos);
            }

            // RotKeys
            uint32 RotCount = 0;
            Ar << RotCount;
            Track.RotKeys.clear();
            Track.RotKeys.reserve(RotCount);
            for (uint32 i = 0; i < RotCount; ++i)
            {
                FQuat Rot;
                Ar << Rot;
                Track.RotKeys.push_back(Rot);
            }

            // ScaleKeys
            uint32 ScaleCount = 0;
            Ar << ScaleCount;
            Track.ScaleKeys.clear();
            Track.ScaleKeys.reserve(ScaleCount);
            for (uint32 i = 0; i < ScaleCount; ++i)
            {
                FVector Scale;
                Ar << Scale;
                Track.ScaleKeys.push_back(Scale);
            }

            // KeyTimes
            uint32 TimeCount = 0;
            Ar << TimeCount;
            Track.KeyTimes.clear();
            Track.KeyTimes.reserve(TimeCount);
            for (uint32 i = 0; i < TimeCount; ++i)
            {
                float Time;
                Ar.Serialize(&Time, sizeof(float));
                Track.KeyTimes.push_back(Time);
            }
        }
        return Ar;
    }
};

struct FBoneAnimationTrack
{
    FName BoneName;
    int32 BoneIndex = -1;
    FRawAnimSequenceTrack InternalTrack; // 실제 애니메이션 데이터

    bool IsValid() const
    {
        return BoneIndex >= 0 && InternalTrack.HasAnyKeys();
    }

    friend FArchive& operator<<(FArchive& Ar, FBoneAnimationTrack& Track)
    {
        Ar << Track.BoneName;
        Ar << Track.BoneIndex;
        Ar << Track.InternalTrack;
        return Ar;
    }
};

// === Animation Float Curves ===
struct FFloatCurveKey
{
    float Time = 0.0f;   // seconds
    float Value = 0.0f;  // scalar

    friend FArchive& operator<<(FArchive& Ar, FFloatCurveKey& Key)
    {
        Ar.Serialize(&Key.Time, sizeof(float));
        Ar.Serialize(&Key.Value, sizeof(float));
        return Ar;
    }
};

struct FCurveTrack
{
    FName CurveName;
    TArray<FFloatCurveKey> Keys;

    bool HasAnyKeys() const { return !Keys.empty(); }

    // 선형 보간으로 커브 값을 평가합니다. 키가 없으면 0, 한 개면 그 값을 반환합니다.
    float Evaluate(float InTime) const
    {
        if (Keys.empty())
            return 0.0f;
        if (Keys.size() == 1)
            return Keys[0].Value;

        // 시간 범위 클램프
        if (InTime <= Keys[0].Time)
            return Keys[0].Value;
        if (InTime >= Keys.back().Time)
            return Keys.back().Value;

        // 선형 탐색 (키 수가 많아지면 이진 탐색로 변경 고려)
        for (size_t i = 0; i + 1 < Keys.size(); ++i)
        {
            const float t0 = Keys[i].Time;
            const float t1 = Keys[i + 1].Time;
            if (InTime >= t0 && InTime <= t1)
            {
                const float v0 = Keys[i].Value;
                const float v1 = Keys[i + 1].Value;
                const float span = (t1 - t0);
                const float alpha = (span > KINDA_SMALL_NUMBER) ? ((InTime - t0) / span) : 0.0f;
                return FMath::Lerp(v0, v1, alpha);
            }
        }
        return Keys.back().Value; // fallback
    }

    friend FArchive& operator<<(FArchive& Ar, FCurveTrack& Track)
    {
        Ar << Track.CurveName;

        if (Ar.IsSaving())
        {
            uint32 KeyCount = static_cast<uint32>(Track.Keys.size());
            Ar << KeyCount;
            for (auto& Key : Track.Keys)
            {
                Ar << Key;
            }
        }
        else if (Ar.IsLoading())
        {
            uint32 KeyCount = 0;
            Ar << KeyCount;
            Track.Keys.clear();
            Track.Keys.reserve(KeyCount);
            for (uint32 i = 0; i < KeyCount; ++i)
            {
                FFloatCurveKey Key;
                Ar << Key;
                Track.Keys.push_back(Key);
            }
        }
        return Ar;
    }
};

//
//struct FFloatCurveKey
//{
//    float Time;
//    float Value;
//};
//
//struct FCurveTrack
//{
//    FName CurveName;        
//    TArray<FFloatCurveKey> Keys;
//};
