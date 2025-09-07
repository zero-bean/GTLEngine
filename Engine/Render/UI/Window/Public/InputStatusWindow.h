#pragma once
#include "UIWindow.h"

/**
 * @brief 키 입력 상태를 표시하는 UI 윈도우
 * 기존에 있던 Key Input Status를 분리한 클래스
 */
class UInputStatusWindow : public UUIWindow
{
public:
	UInputStatusWindow();
	void Initialize() override;
	void Render() override;
	void Update() override;

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
