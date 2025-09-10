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
	UObjectPicker(UCamera& InCamera);
	void SetCamera(UCamera& Camera);
	EGizmoDirection PickGizmo(const FRay& WorldRay, UGizmo& Gizmo, FVector4& CollisionPoint);
	UPrimitiveComponent* PickPrimitive( const FRay& WorldRay, TArray<UPrimitiveComponent*> Candidate, float* Distance);
	bool IsCollideWithPlane(FVector4 PlanePoint, FVector4 Normal, FVector4& PointOnPlane);

private:
	bool IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, float* Distance);


	UCamera& Camera;
};
