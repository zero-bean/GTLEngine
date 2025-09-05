#pragma once
#include "Core/Public/Object.h"

class UInputManager :
	public UObject
{
DECLARE_SINGLETON(UInputManager)

public:
	void Update();
	void UpdateMousePosition();
	void ProcessKeyMessage(UINT InMessage, WPARAM WParam, LPARAM LParam);

	bool IsKeyDown(EKeyInput InKey) const;
	bool IsKeyPressed(EKeyInput InKey) const;
	bool IsKeyReleased(EKeyInput InKey) const;

	// Key Status
	EKeyStatus GetKeyStatus(EKeyInput InKey) const;
	TArray<EKeyInput> GetKeysByStatus(EKeyStatus InStatus) const;

	TArray<EKeyInput> GetPressedKeys() const;
	TArray<EKeyInput> GetNewlyPressedKeys() const;
	TArray<EKeyInput> GetReleasedKeys() const;

	// Window Focus Management
	void SetWindowFocus(bool bInFocused);
	bool IsWindowFocused() const { return bIsWindowFocused; }

	// Helper Function
	static const wchar_t* KeyInputToString(EKeyInput InKey);

	// Getter
	const FVector& GetMousePosition() const { return CurrentMousePosition; }
	const FVector& GetMouseDelta() const { return MouseDelta; }

private:
	// Key Status
	TMap<EKeyInput, bool> CurrentKeyState;
	TMap<EKeyInput, bool> PreviousKeyState;
	TMap<int, EKeyInput> VirtualKeyMap;

	// Mouse Position
	FVector CurrentMousePosition;
	FVector PreviousMousePosition;
	FVector MouseDelta;

	// Window Focus
	bool bIsWindowFocused;

	void InitializeKeyMapping();
};
