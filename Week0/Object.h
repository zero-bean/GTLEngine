#pragma once
#include "pch.h"
#include "ICollide.h"

class Renderer;

class Object :public ICollide
{
public:
	Object() {};
	virtual ~Object() {};

public:
	virtual void Initialize(const Renderer& renderer) = 0;
	virtual void Update(Renderer& renderer) = 0;
	virtual void Render(Renderer& renderer) = 0;
	virtual void Release() = 0;




protected:
	ID3D11Buffer* VertexBuffer{};
	ID3D11Buffer* ConstantBuffer{};

	FVector3 WorldPosition{};

	float Scale{};
	int  NumVertices{};
};

