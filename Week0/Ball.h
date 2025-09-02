#pragma once
#include "pch.h"
#include "ICollide.h"

class Renderer;


class Ball : protected ICollide
{
public:
    Ball();
    ~Ball();


private:
    Ball(const Ball&);
    Ball& operator=(const Ball&);

public:
    void Initialize();
    void Update();
    void Release();
    void Render(Renderer& Renderer);

public:
    // getter
    inline FVector3             GetWorldPosition()const { return WorldPosition; }
    inline FVector3             GetVelocity()const { return Velocity; }
    inline float                GetRadius()const { return Radius; }
    inline float                GetMass()const { return Mass; }


    //setter
    inline void                 SetWorldPosition(const FVector3& InNewPosition) { WorldPosition = InNewPosition; }
    inline void                 SetVelocity(const FVector3& newVelocity) { Velocity = newVelocity; }
    inline void                 SetRadius(float newRadius) { Radius = newRadius; }
    inline void                 SetMass(float newMass) { Mass = newMass; }

private:
    FVector3 WorldPosition{};
    FVector3 Velocity{};

    eBallType BallType;
    float Radius{};
    float Mass{100.0f};

};

