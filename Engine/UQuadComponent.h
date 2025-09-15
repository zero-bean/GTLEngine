#pragma once
#include "stdafx.h"
#include "URenderer.h"
#include "UPrimitiveComponent.h"
#include "VertexPosColor.h"
#include "Vector.h"
class UQuadComponent : public UPrimitiveComponent
{
	DECLARE_UCLASS(UQuadComponent, UPrimitiveComponent)
private:
	bool IsManageable() override { return true; }

public:
	UQuadComponent(FVector pos = { 0, 0, 0 }, FVector rot = { 0, 0, 0 }, FVector scl = { 1, 1, 1 })
		:UPrimitiveComponent(pos, rot, scl)
	{
		// A) 생성자에서 바로 billboard on (항상 billboard로 쓸 Quad라면 추천)
		// 우선은 이 방법을 사용한다.
		SetBillboardEnabled(true);
		SetBillboardSize(64.0f, 64.0f);  // Mode=1(픽셀고정)일 때 픽셀 크기, Mode=0이면 월드 크기
		// 필요하면 디폴트 텍스처도 여기서 SetBillboardTexture(...)로 세팅
	}

	// B) 메쉬 로드 이후 보장하고 싶으면 Init도 오버라이드 (선택)
	bool Init(UMeshManager* meshManager);
};