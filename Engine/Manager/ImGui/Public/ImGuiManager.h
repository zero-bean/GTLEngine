#pragma once
#include "Core/Public/Object.h"

/**
 * @brief ImGui 전체를 관리하는 매니저 클래스
 * TODO(KHJ): UI를 TArray로 관리할 수 있을 거 같음
 */
class UImGuiManager :
	public UObject
{
DECLARE_SINGLETON(UImGuiManager)

public:
	static void Init(HWND InWindowHandle);
	static void Release();
	static void Render();

	static LRESULT WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
