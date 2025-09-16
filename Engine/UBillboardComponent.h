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
	float Spacing = 0.0f;
};