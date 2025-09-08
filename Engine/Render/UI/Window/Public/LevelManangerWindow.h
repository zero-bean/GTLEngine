#pragma once
#include "UIWindow.h"

class ULevelManager;

/**
 * @brief Level Save & Load 기능을 제공하는 UI Window
 * Windows API File Dialog를 사용하여 레벨을 저장하고 불러올 수 있도록 처리
 */
class ULevelManagerWindow : public UUIWindow
{
public:
	void Initialize() override;
	void Render() override;

	// Special Member Function
    ULevelManagerWindow();
    ~ULevelManagerWindow() override = default;

private:
    path OpenSaveFileDialog();
    path OpenLoadFileDialog();

    void SaveLevel(const FString& InFilePath);
    void LoadLevel(const FString& InFilePath);
    void CreateNewLevel();

    ULevelManager* LevelManager;

    char NewLevelNameBuffer[256] = "NewLevel";
    FString StatusMessage;
    float StatusMessageTimer = 0.0f;

    static constexpr float STATUS_MESSAGE_DURATION = 3.0f;
};
