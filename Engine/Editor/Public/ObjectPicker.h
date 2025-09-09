#pragma once
#include "Editor/Public/Gizmo.h"

class UPrimitiveComponent;
class AActor;
class ULevel;
class UCamera;
class UGizmo;

class UObjectPicker : public UObject
{
public:
	void SetCamera(UCamera* Camera);
	void RayCast(ULevel* InLevel, UGizmo& InGizmo);
	AActor* PickActor(ULevel* Level, const FRay& WorldRay, float* ShortedDistance);
	EGizmoDirection PickGizmo(const FRay& WorldRay, UGizmo& Gizmo, float* GizmoDistance);
	bool IsCollideWithPlane(FVector4 PlanePoint, FVector4 PerpenVectorToPlane, FVector4 PointOnPlane);

private:
	bool IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, float* Distance);


	UCamera* Camera = nullptr;
};
