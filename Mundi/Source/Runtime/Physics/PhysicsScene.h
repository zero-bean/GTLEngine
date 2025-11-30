#pragma once

class FPhysicsScene
{
public:
    void Initialize(FPhysicsSystem* System); // 시스템한테서 PxPhysics 받아와서 씬 생성
    void Tick(float DeltaTime); // simulate(), fetchResults()

    void AddActor(FBodyInstance* Body);
    void RemoveActor(FBodyInstance* Body);

private:
    PxScene* PhysXScene = nullptr; // 실제 시뮬레이션 공간
};
