#pragma once
#include "USceneComponent.h"

class UShowFlagManager;
class UMeshManager; 
class UMaterialManager;
class UBillboardComponent;
class UMesh;
class UMaterial;

class UPrimitiveComponent : public USceneComponent
{
	DECLARE_UCLASS(UPrimitiveComponent, USceneComponent)
public:
	UPrimitiveComponent(FVector loc = { 0,0,0 }, FVector rot = { 0,0,0 }, FVector scl = { 1,1,1 })
		: USceneComponent(loc, rot, scl), Mesh(nullptr), Material(nullptr)
		, Billboard(nullptr) {}
	virtual ~UPrimitiveComponent() override;

	bool bIsSelected = false;

	virtual void Draw(URenderer& renderer, UShowFlagManager* ShowFlagManager);
	virtual void UpdateConstantBuffer(URenderer& renderer);

	// 별도의 초기화 메서드
	virtual bool Init(UMeshManager* meshManager, UMaterialManager* materialManager);

	bool CountOnInspector() override { return true; }

	// Getter //
	UMesh* GetMesh() { return Mesh; }
	// Setter //
	void SetColor(const FVector4& newColor) { Color = newColor; }
	FVector4 GetColor() const { return Color; }

protected:
	UBillboardComponent* Billboard;
	UMesh* Mesh;
	UMaterial* Material;
	FVector4 Color = { 1, 1, 1, 1 };

private:
	void DrawMesh(URenderer& renderer);
	void DrawBillboard(URenderer& renderer);
};
