#pragma once
#include "pch.h"
#include "Object.h"
class Renderer;
class Ball : public Object
{
public:
    Ball();
    ~Ball();
private:
    Ball(const Ball&);
    Ball& operator=(const Ball&);
public:
    void Initialize(Renderer& renderer)override;
    void Update(Renderer& renderer) override;
    void Render(Renderer& renderer) override;
    void Release() override;
    void Collidable() override;
public:
    // getter
    inline FVector3             GetWorldPosition()const { return WorldPosition; }
    inline FVector3             GetVelocity()const { return Velocity; }
    inline float                GetRadius()const { return Radius; }
    inline float                GetMass()const { return Mass; }
    inline eBallType            GetBallType()const { return BallType; }
    inline eBallColor           GetBallColor()const { return BallColor; }
    inline eBallState           GetBallState()const { return BallState; }

    //setter
    inline void                 SetWorldPosition(const FVector3& InNewPosition) { WorldPosition = InNewPosition; }
    inline void                 SetVelocity(const FVector3& newVelocity) { Velocity = newVelocity; }
    inline void                 SetRadius(float newRadius) { Radius = newRadius; }
    inline void                 SetMass(float newMass) { Mass = newMass; }
    inline void                 SetBallType(eBallType InBallType) { BallType = InBallType; }
    inline void                 SetBallState(eBallState InBallState) { BallState = InBallState; }

private:
    FVector3 Velocity{};
    eBallType BallType{};
    float Radius{};
    float Mass{ 100.0f };
    float RotationDeg = 0.0f;
    eBallColor BallColor{};

    eBallState BallState{};
};