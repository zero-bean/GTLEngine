#pragma once
#include "pch.h"
#include "Object.h"


class PlayerArrow : public Object
{
public:
	PlayerArrow();
	~PlayerArrow();


public:
	void Initialize(const Renderer& renderer)override;
	void Update(Renderer& renderer) override;
	void Render(Renderer& renderer) override;
	void Release() override;

	void Collidable() override;

public:
	//getter
	inline float GetDegree()
	{
		return RotationDeg;
	}

	//setter
	inline void SetDegree(float InDegree)
	{
		RotationDeg += InDegree;
		//if (RotationDeg < -DirectX::XM_PI / 2.0f)
		//{
		//	RotationDeg = -DirectX::XM_PI / 2.0f;
		//}
		//else if (RotationDeg > DirectX::XM_PI / 2.0f)
		//{
		//	RotationDeg = DirectX::XM_PI / 2.0f;
		//}
	}

private:
	float RotationDeg = 0.0f;

};

