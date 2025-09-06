#pragma once
#include "pch.h"
#include "core/Public/Object.h"
#include "Render/Public/Renderer.h"
#include "Mesh/Public/ActorComponent.h"
#include "ResourceManager.h"
#include "Global/Constant.h"


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
	FVector RelativeRotation = FVector{ 0,0,0.5f };
	FVector RelativeScale3D = FVector{ 0.3f,0.3f,0.3f };
};

class UPrimitiveComponent : public USceneComponent
{
public:
	const TArray<FVertex>* GetVerticesData() const;
	//void Render(const URenderer& Renderer) const override;

protected:
	const TArray<FVertex>* Vertices;
	ID3D11Buffer* Vertexbuffer;   
	UINT NumVertices;
	EPrimitiveType Type;
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
