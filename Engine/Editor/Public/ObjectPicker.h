#pragma once
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Global/Matrix.h"
#include "Global/CoreTypes.h"

class UPrimitiveComponent;
class AActor;
class ULevel;

class UObjectPicker : public UObject
{
public:
	void RayCast(ULevel* InLevel, UCamera& InCamera, UGizmo& InGizmo);
	AActor* PickActor(ULevel* Level, UCamera& Camera, const FRay& WorldRay, float* ShortedDistance);
	EGizmoDirection PickGizmo(UCamera& Camera, const FRay& WorldRay, UGizmo& Gizmo, float* GizmoDistance);

private:
	static FRay ConvertToWorldRay(UCamera& Camera, float InNDCX, float InNDCY);
	bool IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance, const UCamera& Camera);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, const UCamera& Camera, float* Distance);
};
