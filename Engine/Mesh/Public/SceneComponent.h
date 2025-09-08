#pragma once
#include "Mesh/Public/ActorComponent.h"
#include "ResourceManager.h"


class USceneComponent : public UActorComponent
{
public:
	USceneComponent();

	void SetRelativeLocation(const FVector& Location);
	void SetRelativeRotation(const FVector& Rotation);
	void SetRelativeScale3D(const FVector& Scale);

	const FVector& GetRelativeLocation() const;
	const FVector& GetRelativeRotation() const;
	const FVector& GetRelativeScale3D() const;
private:
	FVector RelativeLocation = FVector{ 0,0,0.5f };
	FVector RelativeRotation = FVector{ 0,0,0.f };
	FVector RelativeScale3D = FVector{ 0.3f,0.3f,0.3f };
};

class UPrimitiveComponent : public USceneComponent
{
public:
	UPrimitiveComponent();

	const TArray<FVertex>* GetVerticesData() const;
	ID3D11Buffer* GetVertexBuffer() const;
	//void Render(const URenderer& Renderer) const override;

	bool IsVisible() const { return bVisible; }
	void SetVisibility(bool visibility) { bVisible = visibility; }

protected:
	const TArray<FVertex>* Vertices = nullptr;
	ID3D11Buffer* Vertexbuffer = nullptr;
	UINT NumVertices = 0;
	EPrimitiveType Type = EPrimitiveType::Cube;

	bool bVisible = true;
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
