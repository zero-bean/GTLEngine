#pragma once
#include "UScene.h"
#include "USphereComp.h"
#include "UGizmoGridComp.h"
class UDefaultScene :
    public UScene
{
private:
    static inline bool IsFirstTime = true;
public:
    void Update(float deltaTime) override;
    bool OnInitialize() override;
};

 