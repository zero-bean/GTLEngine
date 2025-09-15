#pragma once
#include "UEngineSubsystem.h"

class UScene;
class UApplication;

class USceneManager : UEngineSubsystem
{
    DECLARE_UCLASS(USceneManager, UEngineSubsystem)
private:
    UApplication* application;
    UScene* currentScene = nullptr;
public:
    ~USceneManager() override;
    bool Initialize(UApplication* _application);

    UScene* GetScene();
    void SetScene(UScene* scene);
    void ReleaseCurrentScene();

    void RequestExit();
    void LoadScene(const FString& path = "");
    void SaveScene(const FString& path = "");
};

