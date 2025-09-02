#include "Ball.h"

Ball::Ball()
{
}

Ball::~Ball()
{
	Release();
}

void Ball::Initialize()
{
	// 반지름 셋
	Radius = 40.0f * (2.0f / 720.0f);
}

void Ball::Update()
{

}

void Ball::Release()
{
}

void Ball::Render(Renderer& Renderer)
{
}
