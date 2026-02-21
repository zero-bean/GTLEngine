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
	UObjectPicker() = default;
	UPrimitiveComponent* PickPrimitive(const FRay& WorldRay, const TArray<TObjectPtr<UPrimitiveComponent>>& Candidates, float* OutDistance);
	void PickGizmo(UCamera* InActiveCamera, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint);
	bool DoesRayIntersectPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane);
	bool DoesRayIntersectPrimitive_MollerTrumbore(const FRay& InModelRay,
		UPrimitiveComponent* InPrimitive, float* OutShortestDistance) const;

private:
	bool RaycastMeshTrianglesBVH(const FRay& ModelRay, UPrimitiveComponent* InPrimitive, float& InOutDistance, bool& OutHit) const;
	bool DoesRayIntersectPrimitive(UCamera* InActiveCamera, const FRay& InWorldRay, UPrimitiveComponent* InPrimitive, const FMatrix& InModelMatrix, float* OutShortestDistance);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive) const;
	bool DoesRayIntersectTriangle(const FRay& InRay, UPrimitiveComponent* InPrimitive,
		const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, float* OutDistance) const;
};

