#pragma once
#include <PxPhysicsAPI.h>

using namespace physx;

class FPhysicsSystem
{
public:
    static FPhysicsSystem& Get();
    void Initialize();
    void Shutdown();
    void UpdateSimulation(float DeltaTime);

    PxPhysics* GetPhysics() const { return mPhysics; }
    PxScene* GetScene() const { return mScene; }
    PxMaterial* GetDefaultMaterial() const { return mMaterial; }

private:
    FPhysicsSystem();
    ~FPhysicsSystem();
    FPhysicsSystem(const FPhysicsSystem&) = delete;
    FPhysicsSystem& operator=(const FPhysicsSystem&) = delete;
    void CreateTestObjects();

private:
    // --- PhysX Core Objects ---
    PxDefaultAllocator      mAllocator;      // 메모리 할당자
    PxDefaultErrorCallback  mErrorCallback;  // 에러 콜백
    
    PxFoundation* mFoundation = nullptr;
    PxPhysics* mPhysics = nullptr;
    PxDefaultCpuDispatcher* mDispatcher = nullptr; // 멀티스레드 관리자
    PxScene* mScene = nullptr;      // 물리 월드
    PxMaterial* mMaterial = nullptr;   // 기본 재질
    PxPvd* mPvd = nullptr;        // 디버거 연결용
};