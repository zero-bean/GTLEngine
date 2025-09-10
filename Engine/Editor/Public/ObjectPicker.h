#pragma once
#include "Editor/Public/Gizmo.h"

class UPrimitiveComponent;
class AActor;
class ULevel;
class UCamera;
class UGizmo;
struct FRay;

class UObjectPicker : public UObject
{
public:
	UObjectPicker(UCamera& InCamera);
	void SetCamera(UCamera& Camera);
	UPrimitiveComponent* PickPrimitive( const FRay& WorldRay, TArray<UPrimitiveComponent*> Candidate, float* Distance);
	void PickGizmo(const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint);
	bool IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Axis, FVector& PointOnPlane);

private:
	bool IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, float* Distance);

	


	UCamera& Camera;
};
