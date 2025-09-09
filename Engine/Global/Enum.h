#pragma once

enum class EKeyInput : uint8
{
	// 이동 키
	W,
	A,
	S,
	D,
	Q,
	E,

	// 화살표 키
	Up,
	Down,
	Left,
	Right,

	// 액션 키
	Space,
	Enter,
	Esc,
	Tab,
	Shift,
	Ctrl,
	Alt,

	// 숫자 키
	Num0,
	Num1,
	Num2,
	Num3,
	Num4,
	Num5,
	Num6,
	Num7,
	Num8,
	Num9,

	// 마우스
	MouseLeft,
	MouseRight,
	MouseMiddle,

	// 기타
	F1,
	F2,
	F3,
	F4,
	Backspace,
	Delete,

	End
};

enum class EKeyStatus : uint8
{
	Up,        // 눌려있지 않은 상태 (현재 false)
	Pressed,   // 이번 프레임에 눌림 (이전 false, 현재 true)
	Down,      // 눌려있는 상태 (현재 true)
	Released,  // 이번 프레임에 떼어짐 (이전 true, 현재 false)
	Unknown,    // 알 수 없음

	End
};
