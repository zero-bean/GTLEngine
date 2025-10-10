#pragma once

enum class EBoundingVolumeType
{
	None,
	AABB,   // Axis-Aligned Bounding Box
	Sphere,  // Bounding Sphere
	OBB // Oriented Bounding Box
};

class IBoundingVolume
{
public:
	virtual ~IBoundingVolume() = default;
	virtual bool RaycastHit(const FRay& Ray, float* OutDistance) const = 0;
	virtual EBoundingVolumeType GetType() const = 0;
	virtual bool Intersects(const IBoundingVolume& Other) const = 0;
};
