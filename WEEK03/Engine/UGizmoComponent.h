#pragma once
#include "stdafx.h"
#include "UMesh.h"
#include "USceneComponent.h"
#include "Vector.h"

class UMeshManager; // 전방 선언

class UGizmoComponent : public USceneComponent
{
	DECLARE_UCLASS(UGizmoComponent, USceneComponent)
public:
	FQuaternion OriginQuaternion;

	UGizmoComponent(FVector loc = { 0,0,0 }, FVector rot = { 0,0,0 }, FVector scl = { 1,1,1 })
		: USceneComponent(loc, rot, scl), Mesh(nullptr)
	{
	}

	bool bIsSelected = false;

	bool Init(UMeshManager* meshManager);
	bool CountOnInspector() override { return true; }

	FMatrix GetWorldTransform() override;

	virtual void Update(float deltaTime);
	virtual void Draw(URenderer& renderer);
	virtual void DrawOnTop(URenderer& renderer);
	virtual void UpdateConstantBuffer(URenderer& renderer);

	UMesh* GetMesh() { return Mesh; }

	void SetOriginRotation(FVector originRotation)
	{
		OriginQuaternion = FQuaternion::FromEulerXYZDeg(originRotation);
	}

	void SetColor(const FVector4& newColor) { Color = newColor; }
	FVector4 GetColor() const { 
		if (bIsSelected)
			return FVector4(1.0f, 1.0f, 0.0f, 1.0f);
		return Color; 
	}

protected:
	UMesh* Mesh;
	FVector4 Color = { 1, 1, 1, 1 };
};
