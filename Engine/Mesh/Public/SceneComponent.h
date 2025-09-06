#pragma once
#include "pch.h"
#include "Mesh/Public/ActorComponent.h"
#include "ResourceManager.h"


class USceneComponent : public UActorComponent
{
public:
	void SetRelativeLocation(const FVector& Location);
	void SetRelativeRotation(const FVector& Rotation);
	void SetRelativeScale3D(const FVector& Scale);

	FVector& GetRelativeLocation();
	FVector& GetRelativeRotation();
	FVector& GetRelativeScale3D();
private:
	FVector RelativeLocation = FVector{ 0,0,0.5f };
	FVector RelativeRotation = FVector{ 0,0,30.f };
	FVector RelativeScale3D = FVector{ 0.3f,0.3f,0.3f };
};

class UPrimitiveComponent : public USceneComponent
{
public:
	const TArray<FVertex>* GetVerticesData() const;
	ID3D11Buffer* GetVertexBuffer() const;
	//void Render(const URenderer& Renderer) const override;

protected:
	const TArray<FVertex>* Vertices = nullptr;
	ID3D11Buffer* Vertexbuffer = nullptr;
	UINT NumVertices = 0;
	EPrimitiveType Type = EPrimitiveType::Cube;
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
