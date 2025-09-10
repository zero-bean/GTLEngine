#pragma once
#include "UIWindow.h"

/**
 * @brief
 * 기존에 있던 Key Input Status를 분리한 클래스
 */
class UExperimentalFeatureWindow : public UUIWindow
{
public:
	UExperimentalFeatureWindow();
	void Initialize() override;
	// void Render() override;
	// void Update() override;

private:
	// 키 입력 히스토리
	TArray<FString> RecentKeyPresses;

	// 마우스 관련
	FVector LastMousePosition;
	FVector MouseDelta;

	// 키 입력 통계
	TMap<FString, int> KeyPressCount;

	void AddKeyToHistory(const FString& InKeyName);
	static void RenderKeyList(const TArray<EKeyInput>& InPressedKeys);
	void RenderMouseInfo() const;
	void RenderKeyStatistics();

	static void RenderControlHelp();
};
