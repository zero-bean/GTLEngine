#pragma once
#include "../Engine/core/Public/Object.h"
#include "../Engine/pch.h"
#include "../Engine/Render/Public/Renderer.h"
#include "Mesh/Public/ActorComponent.h"
#include "ResourceManager.h"


class USceneComponent : public 
{
public:
	void SetRelativeLocation(const FVector& Location);
	void SetRelativeRotation(const FVector& Rotation);
	void SetRelativeScale3D(const FVector& Scale);

	const FVector& GetRelativeLocation();
	const FVector& GetRelativeRotation();
	const FVector& GetRelativeScale3D();
private:
	FVector RelativeLocation = FVector{ 0,0,0 };
	FVector RelativeRotation = FVector{ 0,0,0 };
	FVector RelativeScale3D = FVector{ 0,0,0 };
};

class UPrimitiveComponent : public USceneComponent
{
public:
	const TArray<FVertexSimple>* GetVerticesData() const;
	const void Render(URenderer& Renderer) const;

protected:
	const TArray<FVertexSimple>* Vertices;
	ID3D11Buffer* Vertexbuffer;
	UINT NumVertices;
	EPrimitiveType Type;
};

class UCubeComp : public UPrimitiveComponent
{
public:
	UCubeComp(); 

};

class USphereComp : public UPrimitiveComponent
{
public:
	USphereComp();
};
