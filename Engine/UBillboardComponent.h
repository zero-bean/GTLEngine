#pragma once
#include "USceneComponent.h"

class UMeshManager;
class UMesh;
class URenderer;
class UPrimitiveComponent;

class UBillboardComponent : public USceneComponent
{
	DECLARE_UCLASS(UBillboardComponent, USceneComponent)
public:
	UBillboardComponent(FVector loc = { 0,0,0 }, FVector rot = { 0,0,0 }, FVector scl = { 1,1,1 })
		: USceneComponent(loc, rot, scl) { }
	virtual ~UBillboardComponent();

	bool Init(UMeshManager* meshManager);
	void Draw(URenderer& renderer);
	void UpdateConstantBuffer(URenderer& renderer);

	UMesh* GetMesh();

	void SetOwner(UPrimitiveComponent* inOwner);
	void SetDigitSize(float w, float h) { DigitW = w; DigitH = h; }
	void SetSpacing(float s) { Spacing = s; }

private:
	UMesh* mesh;
	UPrimitiveComponent* owner;
	std::string TextDigits;
	float DigitW = 0.16f;
	float DigitH = 0.36f;
	float Spacing = 0.01f;

	// 폰트 추출 전용 => 필요한 사람이 확장할 것
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