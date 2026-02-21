#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include "Global/Types.h"
#include "Core/Public/Name.h"
#include <Texture/Public/Material.h>
#include "Global/CameraTypes.h"

//struct BatchLineContants
//{
//	float CellSize;
//	//FMatrix BoundingBoxModel;
//	uint32 ZGridStartIndex; // 인덱스 버퍼에서, z방향쪽 그리드가 시작되는 인덱스
//	uint32 BoundingBoxStartIndex; // 인덱스 버퍼에서, 바운딩박스가 시작되는 인덱스
//};

#define HAS_DIFFUSE_MAP	 (1 << 0)
#define HAS_AMBIENT_MAP	 (1 << 1)
#define HAS_SPECULAR_MAP (1 << 2)
#define HAS_NORMAL_MAP	 (1 << 3)
#define HAS_ALPHA_MAP	 (1 << 4)
#define HAS_BUMP_MAP	 (1 << 5)

struct FMaterialConstants
{
	FVector4 Ka;
	FVector4 Kd;
	FVector4 Ks;
	float Ns;
	float Ni;
	float D;
	uint32 MaterialFlags;
	float Time; // Time in seconds
};

struct FVertex
{
	FVector Position;
	FVector4 Color;
};

struct FNormalVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color;
	FVector2 TexCoord;
	FVector4 Tangent;  // XYZ: Tangent, W: Handedness(+1/-1)
};

struct FRay
{
	FVector4 Origin;
	FVector4 Direction;
};

/**
 * @brief Render State Settings for Actor's Component
 */
struct FRenderState
{
	ECullMode CullMode = ECullMode::None;
	EFillMode FillMode = EFillMode::Solid;
};

/**
 * @brief 변환 정보를 담는 구조체
 */
struct FTransform
{
	/** 변환의 벡터 이동 */
	FVector Translation;

	/** 변환의 쿼터니언 회전 */
	FQuaternion Rotation;

	/** 3D 벡터 스케일 (로컬 공간에서만 적용됨) */
	FVector Scale;

	FTransform()
		: Translation(FVector::ZeroVector())
		, Rotation(FQuaternion::Identity())
		, Scale(FVector::OneVector())
	{
	}

	/**
	 * @brief 트랜스폼을 스케일을 포함하여 4x4 행렬로 변환한다. (S * R * T)
	 */
	FMatrix ToMatrixWithScale() const
	{
		FMatrix T = FMatrix::TranslationMatrix(Translation);
		FMatrix R = Rotation.ToRotationMatrix();
		FMatrix S = FMatrix::ScaleMatrix(Scale);

		// S * R * T (Row-major 기준)
		return S * R * T;
	}

	FMatrix ToInverseMatrixWithScale() const
	{
		return ToMatrixWithScale().Inverse();
	}

	/**
	 * @brief 트랜스폼을 스케일을 제외한 4x4 행렬로 변환한다. (R * T)
	 */
	FMatrix ToMatrixNoScale() const
	{
		FMatrix T = FMatrix::TranslationMatrix(Translation);
		FMatrix R = Rotation.ToRotationMatrix();

		// R * T (Row-major 기준)
		return R * T;
	}

	FMatrix ToInverseMatrixNoScale() const
	{
		return ToMatrixNoScale().Inverse();
	}

	FTransform operator*(const FTransform& Other) const
	{
		FTransform Result;

		Result.Scale = Other.Scale * Scale;

		Result.Rotation = Other.Rotation * Rotation ;

		Result.Translation = Other.Translation + Other.Rotation.RotateVector(Other.Scale * Translation);

		return Result;
	}
};

/**
 * @brief 2차원 좌표의 정보를 담는 구조체
 */
struct FPoint
{
	INT X = 0;
	INT Y = 0;
	constexpr FPoint(LONG InX, LONG InY) : X(InX), Y(InY)
	{
	}
};

/**
 * @brief 윈도우를 비롯한 2D 화면의 정보를 담는 구조체
 */
struct FRect
{
	LONG Left = 0;
	LONG Top = 0;
	LONG Width = 0;
	LONG Height = 0;

	LONG GetRight() const { return Left + Width; }
	LONG GetBottom() const { return Top + Height; }
};

struct FAmbientLightInfo
{
	FVector4 Color;
	float Intensity;
	FVector Padding;
};

struct FDirectionalLightInfo
{
	FVector4 Color;
	FVector Direction;
	float Intensity;

	// Shadow parameters
	// FMatrix LightViewProjection;
	uint32 CastShadow;           // 0 or 1
	uint32 ShadowModeIndex;
	float ShadowBias;
	float ShadowSlopeBias;
	float ShadowSharpen;
	float Resolution;
	FVector2 Padding;
};

//StructuredBuffer: 16-byte alignment required (FVector4 alignment)
struct FPointLightInfo
{
	FVector4 Color;
	FVector Position;
	float Intensity;
	float Range;
	float DistanceFalloffExponent;

	// Shadow parameters
	uint32 CastShadow;
	uint32 ShadowModeIndex;
	float ShadowBias;
	float ShadowSlopeBias;
	float ShadowSharpen;
	float Resolution;
};

//StructuredBuffer padding 없어도됨
struct FSpotLightInfo
{
	// Point Light와 공유하는 속성 (필드 순서 맞춤)
	FVector4 Color;
	FVector Position;
	float Intensity;
	float Range;
	float DistanceFalloffExponent;

	// SpotLight 고유 속성
	float InnerConeAngle;
	float OuterConeAngle;
	float AngleFalloffExponent;
	FVector Direction;

	// Shadow parameters
	FMatrix LightViewProjection;
	uint32 CastShadow;
	uint32 ShadowModeIndex;
	float ShadowBias;
	float ShadowSlopeBias;
	float ShadowSharpen;
	float Resolution;
	FVector2 Padding;
};

struct FGlobalLightConstant
{
	FAmbientLightInfo Ambient;
	FDirectionalLightInfo Directional;
};
