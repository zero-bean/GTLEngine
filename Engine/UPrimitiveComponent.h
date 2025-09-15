#pragma once
#include "stdafx.h"
#include "UMesh.h"
#include "USceneComponent.h"
#include "Vector.h"
#include "UClass.h"

class UMeshManager; // 전방 선언


class UPrimitiveComponent : public USceneComponent
{
	DECLARE_UCLASS(UPrimitiveComponent, USceneComponent)
protected:
	UMesh* mesh;
	FVector4 Color = { 1, 1, 1, 1 };
	ID3D11ShaderResourceView* BillboardSRV = nullptr; // 텍스처가 있으면 바인딩
public:
	UPrimitiveComponent(FVector loc = { 0,0,0 }, FVector rot = { 0,0,0 }, FVector scl = { 1,1,1 })
		: USceneComponent(loc, rot, scl), mesh(nullptr)
	{
	}

	bool bIsSelected = false;

	virtual void Draw(URenderer& renderer);
	virtual void UpdateConstantBuffer(URenderer& renderer);
	virtual ~UPrimitiveComponent() {}

	// 별도의 초기화 메서드
	virtual bool Init(UMeshManager* meshManager);

	bool CountOnInspector() override { return true; }

	UMesh* GetMesh() { return mesh; }

	void SetColor(const FVector4& newColor) { Color = newColor; }
	FVector4 GetColor() const { return Color; }
};
