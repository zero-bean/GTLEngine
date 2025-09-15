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
		:UPrimitiveComponent(pos, rot, scl) {}

	virtual void UpdateConstantBuffer(URenderer& renderer) override;

	// font_default 전용 => 필요한 사람이 확장할 것
	inline FSlicedUV GetFontUV(const int id, float padPixel = 0.f)
	{
		// 이미지 사이즈 정보
		const int atlasWidth = 512;
		const int atlasHeight = 512;
		const int cols = 16;
		const int rows = 16;
		const int cell = 32;
		// 픽셀 좌표
		const int cx = id % cols;
		const int cy = id / rows;
		const float x = float(cx * cell);
		const float y = float(cy * cell);
		const float w = float(cell);
		const float h = float(cell);

		float u0 = (x + padPixel) / float(atlasWidth);
		float v0 = (y + padPixel) / float(atlasHeight);
		float u1 = (x + w - padPixel) / float(atlasWidth);
		float v1 = (y + h - padPixel) / float(atlasHeight);

		return FSlicedUV(u0, v0, u1, v1);
	}
};