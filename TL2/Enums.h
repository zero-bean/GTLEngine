//#ifndef UE_ENUMS_H
//#define UE_ENUMS_H
#pragma once
#include "UEContainer.h"
//#include "Enums.h"
#include "Archive.h"
#include <d3d11.h>
#include "Name.h"

struct FObjMaterialInfo
{
    // Diffuse Scalar
    // Diffuse Texture
    // other...

    // Unspecified properties designated by negative values
    int32 IlluminationModel = 2;  // illum. Default illumination model to Phong for non-Pbr materials

    FVector DiffuseColor = FVector::One(); // Kd
    FVector AmbientColor = FVector::One(); // Ka
    FVector SpecularColor = FVector::One(); // Ks
    FVector EmissiveColor = FVector::One(); // Ke

    FName DiffuseTextureFileName;
    FName NormalTextureFileName;
    FString AmbientTextureFileName;
    FString SpecularTextureFileName;
    FString EmissiveTextureFileName;
    FString TransparencyTextureFileName;
    FString SpecularExponentTextureFileName;

    FVector TransmissionFilter = FVector::One(); // Tf

    float OpticalDensity = -1.f; // Ni
    float Transparency = -1.f; // Tr Or d
    float SpecularExponent = -1.f; // Ns

    FString MaterialName;

    friend FArchive& operator<<(FArchive& Ar, FObjMaterialInfo& Info)
    {
        Ar << Info.IlluminationModel;
        Ar << Info.DiffuseColor;
        Ar << Info.AmbientColor;
        Ar << Info.SpecularColor;
        Ar << Info.EmissiveColor;

        if (Ar.IsSaving())
        {
            Serialization::WriteString(Ar, Info.DiffuseTextureFileName.ToString());
            Serialization::WriteString(Ar, Info.AmbientTextureFileName);
            Serialization::WriteString(Ar, Info.SpecularTextureFileName);
            Serialization::WriteString(Ar, Info.EmissiveTextureFileName);
            Serialization::WriteString(Ar, Info.TransparencyTextureFileName);
            Serialization::WriteString(Ar, Info.SpecularExponentTextureFileName);
            Serialization::WriteString(Ar, Info.NormalTextureFileName.ToString());
        }
        else if (Ar.IsLoading())
        {
            FString DiffuseTextureFileString;
            Serialization::ReadString(Ar, DiffuseTextureFileString);
            Info.DiffuseTextureFileName = FName(DiffuseTextureFileString);
            Serialization::ReadString(Ar, Info.AmbientTextureFileName);
            Serialization::ReadString(Ar, Info.SpecularTextureFileName);
            Serialization::ReadString(Ar, Info.EmissiveTextureFileName);
            Serialization::ReadString(Ar, Info.TransparencyTextureFileName);
            Serialization::ReadString(Ar, Info.SpecularExponentTextureFileName);

            FString NormalTextureFileString; 
            Serialization::ReadString(Ar, NormalTextureFileString); 
            Info.NormalTextureFileName = FName(NormalTextureFileString);
        }

        Ar << Info.TransmissionFilter;
        Ar << Info.OpticalDensity;
        Ar << Info.Transparency;
        Ar << Info.SpecularExponent;

        if (Ar.IsSaving())
            Serialization::WriteString(Ar, Info.MaterialName);
        else if (Ar.IsLoading())
            Serialization::ReadString(Ar, Info.MaterialName);

        return Ar;
    }
};

// ---- FObjMaterialInfo 전용 Serialization 특수화 ----
namespace Serialization {
    template<>
    inline void WriteArray<FObjMaterialInfo>(FArchive& Ar, const TArray<FObjMaterialInfo>& Arr) {
        uint32 Count = (uint32)Arr.size();
        Ar << Count;
        for (auto& Mat : Arr) Ar << const_cast<FObjMaterialInfo&>(Mat);
    }

    template<>
    inline void ReadArray<FObjMaterialInfo>(FArchive& Ar, TArray<FObjMaterialInfo>& Arr) {
        uint32 Count;
        Ar << Count;
        Arr.resize(Count);
        for (auto& Mat : Arr) Ar << Mat;
    }
}

struct FGroupInfo
{
    uint32 StartIndex;
    uint32 IndexCount;
    //FObjMaterialInfo MaterialInfo;
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

struct FNormalVertex
{
    FVector Pos;
    FVector Normal;
    FVector4 Color;
    FVector2D Tex;
    FVector Tangent; 
    FVector Bitangent; 

    friend FArchive& operator<<(FArchive& Ar, FNormalVertex& Vtx)
    {
        Ar << Vtx.Pos;
        Ar << Vtx.Normal;
        Ar << Vtx.Color;
        Ar << Vtx.Tex;
        Ar << Vtx.Tangent;
        Ar << Vtx.Bitangent;
        return Ar;
    }
};

//// Cooked Data
struct FStaticMesh
{
    FString PathFileName;

    TArray<FNormalVertex> Vertices;
    TArray<uint32> Indices;
    // to do: 여러가지 추가(ex: material 관련)
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

struct FMeshData
{
	// 중복 없는 정점
	TArray<FVector> Vertices;//also can be billboard world position
	// 정점 인덱스
	TArray<uint32> Indices;
    // 중복 없는 정점
    TArray<FVector4> Color;//also can be UVRect
    // UV 좌표
    TArray<FVector2D> UV;//also can be Billboard size
    // 노말 좌표
    TArray<FVector> Normal;
    // 탄젠트
    TArray<FVector> Tangent;
};

enum class EPrimitiveTopology
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip
};

struct FResourceData
{
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    uint32 VertexCount = 0;     // 정점 개수
    uint32 IndexCount = 0;     // 버텍스 점의 개수 
    uint32 ByteWidth = 0;       // 전체 버텍스 데이터 크기 (sizeof(FVertexSimple) * VertexCount)
    uint32 Stride = 0;
    uint32 Offset = 0;
    EPrimitiveTopology Topology = EPrimitiveTopology::TriangleList;
    D3D11_PRIMITIVE_TOPOLOGY Topol = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};

struct FTextureData
{
    ID3D11Resource* Texture = nullptr;
    ID3D11ShaderResourceView* TextureSRV = nullptr;
    ID3D11BlendState* BlendState = nullptr;
};

enum class EResourceType
{
    VertexBuffer,
    IndexBuffer
};

enum class EGizmoMode : uint8
{
    Translate,
    Rotate,
    Scale
};
enum class EGizmoSpace : uint8
{
    World,
    Local
};

enum class EKeyInput : uint8
{
    // Keyboard Keys
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Space, Enter, Escape, Tab, Shift, Ctrl, Alt,
    Up, Down, Left, Right,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    
    // Mouse Buttons
    LeftMouse, RightMouse, MiddleMouse, Mouse4, Mouse5,
    
    // Special
    Unknown
};

//TODO EResourceType으로 재정의
enum class ResourceType : uint8
{
    None,

    StaticMesh,
    TextQuad,
    DynamicMesh,
    Shader,
    Texture,
    Material,

    End
};

enum class EVertexLayoutType : uint8
{
    None,

    PositionColor,
    PositionColorTexturNormal,
    PositionColorTexturNormalTangent,
    PositionUV,
    PositionBillBoard,
    PositionCollisionDebug,

    End,
};

enum class EViewModeIndex : uint32
{
    None,

    VMI_Lit,
    VMI_Unlit,
    VMI_Wireframe,
    VMI_SceneDepth,
    VMI_WorldNormal,

    End,
};

enum class EPrimitiveType : uint32
{
    None,

    Default,
    Sphere,

    End,
};

// ESpawnActorType은 더 이상 사용되지 않습니다.
// Reflection 기반 시스템(UClassRegistry::GetSpawnableClasses)을 사용하세요.

/**
 * Show Flag system for toggling rendering features globally
 * Uses bit flags for efficient storage and checking
 */

enum class EEngineShowFlags : uint64
{
    None = 0,

    // Primitive rendering
    SF_Primitives = 1ull << 0,    // Show/hide all primitive geometry
    SF_StaticMeshes = 1ull << 1,  // Show/hide static mesh actors
    SF_Wireframe = 1ull << 2,     // Show wireframe overlay

    // Debug features
    SF_BillboardText = 1ull << 3, // Show/hide UUID text above objects
    SF_BoundingBoxes = 1ull << 4, // Show/hide collision bounds
    SF_Grid = 1ull << 5,          // Show/hide world grid

    // Lighting
    SF_Lighting = 1ull << 6,      // Enable/disable lighting

    // Decal
    SF_Decals = 1ull << 7,         // Show/hide all decal components

    //BVH
    SF_BVH = 1ull << 8,            //Show/hide all BVH

    // Tile Culling Debug
    SF_TileCullingDebug = 1ull << 9,  // Show/hide tile-based light culling debug visualization

    // Light debug lines (Spot/Point)
    SF_LightDebugLines = 1ull << 10,   // Show/hide debug line visualization for point/spot lights

    // Default enabled flags
    SF_DefaultEnabled = SF_Primitives | SF_StaticMeshes | SF_Grid | SF_Decals | SF_BVH | SF_BoundingBoxes,

    // All flags (for initialization/reset)
    SF_All = 0xFFFFFFFFFFFFFFFFull
};

enum class EViewportLayoutMode
{
    SingleMain,
    FourSplit
};


// Bit flag operators for EEngineShowFlags
inline EEngineShowFlags operator|(EEngineShowFlags a, EEngineShowFlags b)
{
    return static_cast<EEngineShowFlags>(static_cast<uint64>(a) | static_cast<uint64>(b));
}

inline EEngineShowFlags operator&(EEngineShowFlags a, EEngineShowFlags b)
{
    return static_cast<EEngineShowFlags>(static_cast<uint64>(a) & static_cast<uint64>(b));
}

inline EEngineShowFlags operator~(EEngineShowFlags a)
{
    return static_cast<EEngineShowFlags>(~static_cast<uint64>(a));
}

inline EEngineShowFlags& operator|=(EEngineShowFlags& a, EEngineShowFlags b)
{
    a = a | b;
    return a;
}

inline EEngineShowFlags& operator&=(EEngineShowFlags& a, EEngineShowFlags b)
{
    a = a & b;
    return a;
}

// Helper function to check if a flag is set
inline bool HasShowFlag(EEngineShowFlags flags, EEngineShowFlags flag)
{
    return (flags & flag) != EEngineShowFlags::None;
}

// enum class를 쓰기 전부터 사용해오던 영향으로 namespace 사용
// 여기선 고증에 맞게 작성함
namespace EEndPlayReason
{
    enum Type
    {
        EndPlayInEditor,
        RemovedFromWorld,
        Destroyed,
        LevelTransition,
        Quit,
    };
}
enum class EComponentWorldTickMode
{
    All,        // 모든 월드에서 (Editor + PIE + Game)
    PIEOnly,    // PIE에서만
    GameOnly,   // 실제 게임에서만
    EditorOnly  // 에디터에서만
};

/// @brief EnumClass를 기본 자료형으로 변환
template<typename EnumClass>
constexpr auto ToUnderlying(EnumClass Enum) noexcept
{
    return static_cast<std::underlying_type_t<EnumClass>>(Enum);
}

/*
 * @brief 기본 자료형을 EnumClass로 변환
 * @tparam EnumClass 변환될 목표 열거형 클래스
 * @tparam Type 입력되는 정수 값 기본 자료형
 */
template<typename EnumClass, typename Type>
constexpr EnumClass ToEnum(Type Value)
{
    return static_cast<EnumClass>(Value);
}

template<typename EnumClass>
constexpr EnumClass& operator++(EnumClass& Enum) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");

    Enum = ToEnum<EnumClass>(ToUnderlying(Enum) + 1);
    
    return Enum;
}

template<typename EnumClass>
constexpr EnumClass operator++(EnumClass& Enum, int Dummy) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");
    EnumClass OriginalEnum = Enum;
    ++Enum;

    return OriginalEnum;
}

template<typename EnumClass>
constexpr EnumClass operator%(EnumClass LHS, EnumClass RHS) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");
    return ToEnum<EnumClass>(ToUnderlying(LHS) % ToUnderlying(RHS));
}

template<typename EnumClass>
constexpr EnumClass operator|(EnumClass LHS, EnumClass RHS) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");
    return ToEnum<EnumClass>(ToUnderlying(LHS) | ToUnderlying(RHS));
}

template<typename EnumClass>
constexpr EnumClass operator&(EnumClass LHS, EnumClass RHS) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");
    return ToEnum<EnumClass>(ToUnderlying(LHS) & ToUnderlying(RHS));
}
template<typename EnumClass>
constexpr EnumClass operator^(EnumClass LHS, EnumClass RHS) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");
    return ToEnum<EnumClass>(ToUnderlying(LHS) ^ ToUnderlying(RHS));
}

template<typename EnumClass>
constexpr EnumClass operator~(EnumClass Value) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");
    return ToEnum<EnumClass>(~ToUnderlying(Value));
}

template<typename EnumClass>
constexpr EnumClass& operator |=(EnumClass& LHS, EnumClass RHS) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");
    LHS = LHS | RHS;
    return LHS;
}

template<typename EnumClass>
constexpr EnumClass& operator &=(EnumClass& LHS, EnumClass RHS) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");
    LHS = LHS & RHS;
    return LHS;
}

template<typename EnumClass>
constexpr EnumClass& operator ^=(EnumClass& LHS, EnumClass RHS) noexcept
{
    static_assert(std::is_enum_v<EnumClass>, "This class must be an enum.");
    LHS = LHS ^ RHS;
    return LHS;
}

//#endif /** UE_ENUMS_H */
