#pragma once
#include "Physics/Public/BoundingVolume.h"
#include "Global/Vector.h"

struct FAABB : public IBoundingVolume
{
	FVector Min;
	FVector Max;

	FAABB() : Min(0.f, 0.f, 0.f), Max(0.f, 0.f, 0.f) {}
	FAABB(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax) {}

	bool RaycastHit() const override;
	EBoundingVolumeType GetType() const override { return EBoundingVolumeType::AABB; }
};
