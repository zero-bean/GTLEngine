#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include <cstdint>

struct FViewProjConstants
{
	FViewProjConstants()
	{
		View = FMatrix::Identity();
		Projection = FMatrix::Identity();
	}

	FMatrix View;
	FMatrix Projection;
};


struct FVertex
{
	FVector Position;
	FVector4 Color;
};

struct FRay
{
	FVector4 Origin;
	FVector4 Direction;
};

/**
 * @brief Component Type Enum
 */
enum class EComponentType : uint8_t
{
	None = 0,

	Actor,
		//ActorComponent Dervied Type

	Scene,
		//SceneComponent Dervied Type

	Primitive,
		//PrimitiveComponent Derived Type

	End = 0xFF
};

/**
 * @brief UObject Primitive Type Enum
 */
enum class EPrimitiveType : uint8_t
{
	None = 0,
	Sphere,
	Cube,
	Triangle,
	Square,
	Torus,
	Arrow,
	CubeArrow,
	Ring,
	Line,

	End = 0xFF
};

/**
 * @brief RasterizerState Enum
 */
enum class ECullMode : uint8_t
{
	Back,
	Front,
	None,

	End = 0xFF
};

enum class EFillMode : uint8_t
{
	WireFrame,
	Solid,

	End = 0xFF
};

/**
 * @brief Render State Settings for Actor's Component 
 */
struct FRenderState
{
	ECullMode CullMode = ECullMode::None;
	EFillMode FillMode = EFillMode::Solid;
};
