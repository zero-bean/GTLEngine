#pragma once

#include "Global/CoreTypes.h"
#include "Global/Vector.h"

#include <cmath>

inline bool RayTriangleIntersectModel(const FRay& ModelRay, const FVector& V0, const FVector& V1, const FVector& V2, float& OutDistance)
{
	const FVector Origin(ModelRay.Origin.X, ModelRay.Origin.Y, ModelRay.Origin.Z);
	const FVector Direction(ModelRay.Direction.X, ModelRay.Direction.Y, ModelRay.Direction.Z);

	const FVector Edge1 = V1 - V0;
	const FVector Edge2 = V2 - V0;

	const FVector PVec = Direction.Cross(Edge2);
	const float Determinant = Edge1.Dot(PVec);

	if (std::fabs(Determinant) < 1e-6f)
	{
		return false;
	}

	const float InvDet = 1.0f / Determinant;

	const FVector TVec = Origin - V0;
	const float U = TVec.Dot(PVec) * InvDet;
	if (U < 0.0f || U > 1.0f)
	{
		return false;
	}

	const FVector QVec = TVec.Cross(Edge1);
	const float V = Direction.Dot(QVec) * InvDet;
	if (V < 0.0f || (U + V) > 1.0f)
	{
		return false;
	}

	const float T = Edge2.Dot(QVec) * InvDet;
	if (T < 0.0f)
	{
		return false;
	}

	OutDistance = T;
	return true;
}

// Variant that uses precomputed E0 = (V1 - V0), E1 = (V2 - V0) for speed and supports pruning with tMax.
inline bool RayTriangleIntersectModelPrecomputed(
    const FRay& ModelRay,
    const FVector& V0,
    const FVector& E0,
    const FVector& E1,
    float tMax,
    float& OutDistance)
{
    const FVector Origin(ModelRay.Origin.X, ModelRay.Origin.Y, ModelRay.Origin.Z);
    const FVector Direction(ModelRay.Direction.X, ModelRay.Direction.Y, ModelRay.Direction.Z);

    const FVector pvec = Direction.Cross(E1);
    const float det = E0.Dot(pvec);

    // Accept both sides (no backface cull), reject near-degenerate triangles
    constexpr float kEps = 1e-8f;
    if (fabsf(det) < kEps) return false;
    const float invDet = 1.0f / det;

    const FVector tvec = Origin - V0;
    const float u = (tvec.Dot(pvec)) * invDet;
    if (u < 0.0f || u > 1.0f) return false;

    const FVector qvec = tvec.Cross(E0);
    const float v = (Direction.Dot(qvec)) * invDet;
    if (v < 0.0f || (u + v) > 1.0f) return false;

    const float t = (E1.Dot(qvec)) * invDet;
    if (t <= 0.0f || t >= tMax) return false;

    OutDistance = t;
    return true;
}
