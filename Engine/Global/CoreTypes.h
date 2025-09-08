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

	Line,

	Gizmo,

	End = 0xFF
};
