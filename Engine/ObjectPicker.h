#pragma once

class AActor;
class ULevel;


class UObjectPicker : public UObject
{
public:
	AActor* PickActor(ULevel* Level, HWND WindowHandle);

private:
	FRay ConvertToWorldRay(int PixelX, int PixelY, int ViewportW, int ViewportH,
		const FViewProjConstants& ViewProjConstantsInverse);
	bool IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, float* Distance);
};
