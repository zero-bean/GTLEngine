#pragma once

class FClientApp;

/**
 * @brief 윈도우 생성 및 메시지 처리를 담당하는 클래스
 */
class FAppWindow
{
public:
	bool Init(HINSTANCE InInstance, int InCmdShow);
	static void InitializeConsole();

	// Getter & Setter
	HWND GetWindowHandle() const { return MainWindowHandle; }
	HINSTANCE GetInstanceHandle() const { return InstanceHandle; }
	void SetTitle(const wstring& InNewTitle) const;

	// Special Member Function
	FAppWindow(FClientApp* InOwner);
	~FAppWindow();

private:
    FClientApp* Owner;
    HINSTANCE InstanceHandle;
    HWND MainWindowHandle;

    // Windows Callback Function
    static LRESULT CALLBACK WndProc(HWND InWindowHandle, UINT InMessage, WPARAM InWParam, LPARAM InLParam);

    // Message Handle Function
    static FAppWindow* GetWindowInstance(HWND InWindowHandle, UINT InMessage, LPARAM InLParam);
};
