#pragma once
#include "Mesh/Public/ActorComponent.h"
#include "ResourceManager.h"

class USceneComponent : public UActorComponent
{
public:
	USceneComponent();

	void SetParentAttachment(USceneComponent* SceneComponent);
	void RemoveChild(USceneComponent* ChildDeleted);

	void MarkAsDirty();

	void SetRelativeLocation(const FVector& Location);
	void SetRelativeRotation(const FVector& Rotation);
	void SetRelativeScale3D(const FVector& Scale);

	const FVector& GetRelativeLocation() const;
	const FVector& GetRelativeRotation() const;
	const FVector& GetRelativeScale3D() const;

	const FMatrix& GetWorldTransformMatrix() const;
	const FMatrix& GetWorldTransformMatrixInverse() const;

private:
	mutable bool bIsTransformDirty = true;
	mutable bool bIsTransformDirtyInverse = true;
	mutable FMatrix WorldTransformMatrix;
	mutable FMatrix WorldTransformMatrixInverse;

	USceneComponent* ParentAttachment = nullptr;
	TArray<USceneComponent*> Children;
	FVector RelativeLocation = FVector{ 0,0,0.f };
	FVector RelativeRotation = FVector{ 0,0,0.f };
	FVector RelativeScale3D = FVector{ 0.3f,0.3f,0.3f };
};

class UPrimitiveComponent : public USceneComponent
{
public:
	UPrimitiveComponent();

	const TArray<FVertex>* GetVerticesData() const;
	ID3D11Buffer* GetVertexBuffer() const;

	void SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology);
	D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;
	//void Render(const URenderer& Renderer) const override;

	bool IsVisible() const { return bVisible; }
	void SetVisibility(bool bVisibility) { bVisible = bVisibility; }

	FVector4 GetColor() const { return Color; }
	void SetColor(const FVector4& InColor) { Color = InColor; }

protected:
	const TArray<FVertex>* Vertices = nullptr;
	ID3D11Buffer* Vertexbuffer = nullptr;
	uint32 NumVertices = 0;
	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	EPrimitiveType Type = EPrimitiveType::Cube;

	bool bVisible = true;

	FVector4 Color = FVector4{ 0.f,0.f,0.f,0.f };
};

class UCubeComponent : public UPrimitiveComponent
{
public:
	UCubeComponent();

};

class USphereComponent : public UPrimitiveComponent
{
public:
	USphereComponent();
};

class ULineComponent : public UPrimitiveComponent
{
public:
	ULineComponent();
};
