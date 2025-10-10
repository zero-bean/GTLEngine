#pragma once
#include "AABB.h"
#include "BoundingVolume.h"

struct FOBB : public IBoundingVolume
{
	FVector Center;
	FVector Extents;
	FMatrix Orientation;

	FOBB()
		: Center(0.f, 0.f, 0.f), Extents(0.f, 0.f, 0.f), Orientation(FMatrix::Identity()) {}

	FOBB(const FVector& InCenter, const FVector& InExtents, const FMatrix& InOrientation)
		: Center(InCenter), Extents(InExtents), Orientation(InOrientation) {}

	bool RaycastHit(const FRay& Ray, float* OutDistance) const override;
	EBoundingVolumeType GetType() const override { return EBoundingVolumeType::OBB; }
	bool Intersects(const IBoundingVolume& Other) const override;

private:
	bool IntersectsAABB(const FAABB& Other) const;
};
