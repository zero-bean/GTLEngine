#include "stdafx.h"
#include "UDefaultScene.h"
#include "URaycastManager.h"

void UDefaultScene::Update(float deltaTime)
{
    UScene::Update(deltaTime);
    static float t = 0.0f;
    t += deltaTime;
}

bool UDefaultScene::OnInitialize()
{
    UScene::OnInitialize();
    if (IsFirstTime)
    {
        // 컴포넌트 생성
        USphereComp* sphere = NewObject<USphereComp>(
            FVector{ 0.0f, 0.0f, 0.0f }, 
            FVector{ 0.0f, 0.0f, 0.0f }, 
            FVector{ 1.0f, 1.0f, 1.0f }
        );

        AddObject(sphere);
        IsFirstTime = false;
    }

	return true;
}
