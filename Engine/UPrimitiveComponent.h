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
	bool    bBillboard = false;  // 이 컴포넌트를 billboard로 그릴지
	float   BillboardSizeX = 64.f; // Mode=1이면 픽셀, Mode=0이면 월드 단위
	float   BillboardSizeY = 64.f;
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
	// 빌보드 세터
	void SetBillboardEnabled(bool b) 
	{ 
		bBillboard = b; 
	}
	void SetBillboardSize(float sx, float sy) 
	{ 
		BillboardSizeX = sx; 
		BillboardSizeY = sy; 
	}
	void SetBillboardTexture(ID3D11ShaderResourceView* srv) 
	{ 
		BillboardSRV = srv;
	}
};
