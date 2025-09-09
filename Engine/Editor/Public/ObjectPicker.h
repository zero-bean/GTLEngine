#pragma once
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Global/Matrix.h"
#include "Global/Vector.h"
#include "Global/CoreTypes.h"
#include <Windows.h>

class UPrimitiveComponent;
class AActor;
class ULevel;


class UObjectPicker : public UObject
{
public:
	void RayCast(ULevel* Level, HWND WindowHandle, UCamera& Camera, UGizmo& Gizmo);
	AActor* PickActor(ULevel* Level, UCamera& Camera, const FRay& WorldRay, float* ShortedDistance);
	EGizmoDirection PickGizmo(UCamera& Camera, const FRay& WorldRay, UGizmo& Gizmo, float* GizmoDistance);

private:
	FRay ConvertToWorldRay(UCamera& Camera, int PixelX, int PixelY, int ViewportW, int ViewportH);
	bool IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance, const UCamera& Camera);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, const UCamera& Camera, float* Distance);
};
